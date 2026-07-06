#include "qcustomplot.h"

QCPPainter::QCPPainter() :
  QPainter(),
  mModes(pmDefault),
  mIsAntialiasing(false)
{

}

QCPPainter::QCPPainter(QPaintDevice *device) :
  QPainter(device),
  mModes(pmDefault),
  mIsAntialiasing(false)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (isActive())
    setRenderHint(QPainter::NonCosmeticDefaultPen);
#endif
}

QCPPainter::~QCPPainter()
{
}

void QCPPainter::setPen(const QPen &pen)
{
  QPainter::setPen(pen);
  if (mModes.testFlag(pmNonCosmetic))
    makeNonCosmetic();
}

void QCPPainter::setPen(const QColor &color)
{
  QPainter::setPen(color);
  if (mModes.testFlag(pmNonCosmetic))
    makeNonCosmetic();
}

void QCPPainter::setPen(Qt::PenStyle penStyle)
{
  QPainter::setPen(penStyle);
  if (mModes.testFlag(pmNonCosmetic))
    makeNonCosmetic();
}

void QCPPainter::drawLine(const QLineF &line)
{
  if (mIsAntialiasing || mModes.testFlag(pmVectorized))
    QPainter::drawLine(line);
  else
    QPainter::drawLine(line.toLine());
}

void QCPPainter::setAntialiasing(bool enabled)
{
  setRenderHint(QPainter::Antialiasing, enabled);
  if (mIsAntialiasing != enabled)
  {
    mIsAntialiasing = enabled;
    if (!mModes.testFlag(pmVectorized))
    {
      if (mIsAntialiasing)
        translate(0.5, 0.5);
      else
        translate(-0.5, -0.5);
    }
  }
}

void QCPPainter::setModes(QCPPainter::PainterModes modes)
{
  mModes = modes;
}

bool QCPPainter::begin(QPaintDevice *device)
{
  bool result = QPainter::begin(device);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (result)
    setRenderHint(QPainter::NonCosmeticDefaultPen);
#endif
  return result;
}

void QCPPainter::setMode(QCPPainter::PainterMode mode, bool enabled)
{
  if (!enabled && mModes.testFlag(mode))
    mModes &= ~mode;
  else if (enabled && !mModes.testFlag(mode))
    mModes |= mode;
}

void QCPPainter::save()
{
  mAntialiasingStack.push(mIsAntialiasing);
  QPainter::save();
}

void QCPPainter::restore()
{
  if (!mAntialiasingStack.isEmpty())
    mIsAntialiasing = mAntialiasingStack.pop();
  else
    qDebug() << Q_FUNC_INFO << "Unbalanced save/restore";
  QPainter::restore();
}

void QCPPainter::makeNonCosmetic()
{
  if (qFuzzyIsNull(pen().widthF()))
  {
    QPen p = pen();
    p.setWidth(1);
    QPainter::setPen(p);
  }
}

QCPScatterStyle::QCPScatterStyle() :
  mSize(6),
  mShape(ssNone),
  mPen(Qt::NoPen),
  mBrush(Qt::NoBrush),
  mPenDefined(false)
{
}

QCPScatterStyle::QCPScatterStyle(ScatterShape shape, double size) :
  mSize(size),
  mShape(shape),
  mPen(Qt::NoPen),
  mBrush(Qt::NoBrush),
  mPenDefined(false)
{
}

QCPScatterStyle::QCPScatterStyle(ScatterShape shape, const QColor &color, double size) :
  mSize(size),
  mShape(shape),
  mPen(QPen(color)),
  mBrush(Qt::NoBrush),
  mPenDefined(true)
{
}

QCPScatterStyle::QCPScatterStyle(ScatterShape shape, const QColor &color, const QColor &fill, double size) :
  mSize(size),
  mShape(shape),
  mPen(QPen(color)),
  mBrush(QBrush(fill)),
  mPenDefined(true)
{
}

QCPScatterStyle::QCPScatterStyle(ScatterShape shape, const QPen &pen, const QBrush &brush, double size) :
  mSize(size),
  mShape(shape),
  mPen(pen),
  mBrush(brush),
  mPenDefined(pen.style() != Qt::NoPen)
{
}

QCPScatterStyle::QCPScatterStyle(const QPixmap &pixmap) :
  mSize(5),
  mShape(ssPixmap),
  mPen(Qt::NoPen),
  mBrush(Qt::NoBrush),
  mPixmap(pixmap),
  mPenDefined(false)
{
}

QCPScatterStyle::QCPScatterStyle(const QPainterPath &customPath, const QPen &pen, const QBrush &brush, double size) :
  mSize(size),
  mShape(ssCustom),
  mPen(pen),
  mBrush(brush),
  mCustomPath(customPath),
  mPenDefined(false)
{
}

void QCPScatterStyle::setSize(double size)
{
  mSize = size;
}

void QCPScatterStyle::setShape(QCPScatterStyle::ScatterShape shape)
{
  mShape = shape;
}

void QCPScatterStyle::setPen(const QPen &pen)
{
  mPenDefined = true;
  mPen = pen;
}

void QCPScatterStyle::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPScatterStyle::setPixmap(const QPixmap &pixmap)
{
  setShape(ssPixmap);
  mPixmap = pixmap;
}

void QCPScatterStyle::setCustomPath(const QPainterPath &customPath)
{
  setShape(ssCustom);
  mCustomPath = customPath;
}

void QCPScatterStyle::applyTo(QCPPainter *painter, const QPen &defaultPen) const
{
  painter->setPen(mPenDefined ? mPen : defaultPen);
  painter->setBrush(mBrush);
}

void QCPScatterStyle::drawShape(QCPPainter *painter, QPointF pos) const
{
  drawShape(painter, pos.x(), pos.y());
}

void QCPScatterStyle::drawShape(QCPPainter *painter, double x, double y) const
{
  double w = mSize/2.0;
  switch (mShape)
  {
    case ssNone: break;
    case ssDot:
    {
      painter->drawLine(QPointF(x, y), QPointF(x+0.0001, y));
      break;
    }
    case ssCross:
    {
      painter->drawLine(QLineF(x-w, y-w, x+w, y+w));
      painter->drawLine(QLineF(x-w, y+w, x+w, y-w));
      break;
    }
    case ssPlus:
    {
      painter->drawLine(QLineF(x-w,   y, x+w,   y));
      painter->drawLine(QLineF(  x, y+w,   x, y-w));
      break;
    }
    case ssCircle:
    {
      painter->drawEllipse(QPointF(x , y), w, w);
      break;
    }
    case ssDisc:
    {
      QBrush b = painter->brush();
      painter->setBrush(painter->pen().color());
      painter->drawEllipse(QPointF(x , y), w, w);
      painter->setBrush(b);
      break;
    }
    case ssSquare:
    {
      painter->drawRect(QRectF(x-w, y-w, mSize, mSize));
      break;
    }
    case ssDiamond:
    {
      painter->drawLine(QLineF(x-w,   y,   x, y-w));
      painter->drawLine(QLineF(  x, y-w, x+w,   y));
      painter->drawLine(QLineF(x+w,   y,   x, y+w));
      painter->drawLine(QLineF(  x, y+w, x-w,   y));
      break;
    }
    case ssStar:
    {
      painter->drawLine(QLineF(x-w,   y, x+w,   y));
      painter->drawLine(QLineF(  x, y+w,   x, y-w));
      painter->drawLine(QLineF(x-w*0.707, y-w*0.707, x+w*0.707, y+w*0.707));
      painter->drawLine(QLineF(x-w*0.707, y+w*0.707, x+w*0.707, y-w*0.707));
      break;
    }
    case ssTriangle:
    {
       painter->drawLine(QLineF(x-w, y+0.755*w, x+w, y+0.755*w));
       painter->drawLine(QLineF(x+w, y+0.755*w,   x, y-0.977*w));
       painter->drawLine(QLineF(  x, y-0.977*w, x-w, y+0.755*w));
      break;
    }
    case ssTriangleInverted:
    {
       painter->drawLine(QLineF(x-w, y-0.755*w, x+w, y-0.755*w));
       painter->drawLine(QLineF(x+w, y-0.755*w,   x, y+0.977*w));
       painter->drawLine(QLineF(  x, y+0.977*w, x-w, y-0.755*w));
      break;
    }
    case ssCrossSquare:
    {
       painter->drawLine(QLineF(x-w, y-w, x+w*0.95, y+w*0.95));
       painter->drawLine(QLineF(x-w, y+w*0.95, x+w*0.95, y-w));
       painter->drawRect(QRectF(x-w, y-w, mSize, mSize));
      break;
    }
    case ssPlusSquare:
    {
       painter->drawLine(QLineF(x-w,   y, x+w*0.95,   y));
       painter->drawLine(QLineF(  x, y+w,        x, y-w));
       painter->drawRect(QRectF(x-w, y-w, mSize, mSize));
      break;
    }
    case ssCrossCircle:
    {
       painter->drawLine(QLineF(x-w*0.707, y-w*0.707, x+w*0.670, y+w*0.670));
       painter->drawLine(QLineF(x-w*0.707, y+w*0.670, x+w*0.670, y-w*0.707));
       painter->drawEllipse(QPointF(x, y), w, w);
      break;
    }
    case ssPlusCircle:
    {
       painter->drawLine(QLineF(x-w,   y, x+w,   y));
       painter->drawLine(QLineF(  x, y+w,   x, y-w));
       painter->drawEllipse(QPointF(x, y), w, w);
      break;
    }
    case ssPeace:
    {
       painter->drawLine(QLineF(x, y-w,         x,       y+w));
       painter->drawLine(QLineF(x,   y, x-w*0.707, y+w*0.707));
       painter->drawLine(QLineF(x,   y, x+w*0.707, y+w*0.707));
       painter->drawEllipse(QPointF(x, y), w, w);
      break;
    }
    case ssPixmap:
    {
      painter->drawPixmap(x-mPixmap.width()*0.5, y-mPixmap.height()*0.5, mPixmap);
      break;
    }
    case ssCustom:
    {
      QTransform oldTransform = painter->transform();
      painter->translate(x, y);
      painter->scale(mSize/6.0, mSize/6.0);
      painter->drawPath(mCustomPath);
      painter->setTransform(oldTransform);
      break;
    }
  }
}

QCPLayer::QCPLayer(QCustomPlot *parentPlot, const QString &layerName) :
  QObject(parentPlot),
  mParentPlot(parentPlot),
  mName(layerName),
  mIndex(-1),
  mVisible(true)
{

}

QCPLayer::~QCPLayer()
{

  while (!mChildren.isEmpty())
    mChildren.last()->setLayer(0);

  if (mParentPlot->currentLayer() == this)
    qDebug() << Q_FUNC_INFO << "The parent plot's mCurrentLayer will be a dangling pointer. Should have been set to a valid layer or 0 beforehand.";
}

void QCPLayer::setVisible(bool visible)
{
  mVisible = visible;
}

void QCPLayer::addChild(QCPLayerable *layerable, bool prepend)
{
  if (!mChildren.contains(layerable))
  {
    if (prepend)
      mChildren.prepend(layerable);
    else
      mChildren.append(layerable);
  } else
    qDebug() << Q_FUNC_INFO << "layerable is already child of this layer" << reinterpret_cast<quintptr>(layerable);
}

void QCPLayer::removeChild(QCPLayerable *layerable)
{
  if (!mChildren.removeOne(layerable))
    qDebug() << Q_FUNC_INFO << "layerable is not child of this layer" << reinterpret_cast<quintptr>(layerable);
}

QCPLayerable::QCPLayerable(QCustomPlot *plot, QString targetLayer, QCPLayerable *parentLayerable) :
  QObject(plot),
  mVisible(true),
  mParentPlot(plot),
  mParentLayerable(parentLayerable),
  mLayer(0),
  mAntialiased(true)
{
  if (mParentPlot)
  {
    if (targetLayer.isEmpty())
      setLayer(mParentPlot->currentLayer());
    else if (!setLayer(targetLayer))
      qDebug() << Q_FUNC_INFO << "setting QCPlayerable initial layer to" << targetLayer << "failed.";
  }
}

QCPLayerable::~QCPLayerable()
{
  if (mLayer)
  {
    mLayer->removeChild(this);
    mLayer = 0;
  }
}

void QCPLayerable::setVisible(bool on)
{
  mVisible = on;
}

bool QCPLayerable::setLayer(QCPLayer *layer)
{
  return moveToLayer(layer, false);
}

bool QCPLayerable::setLayer(const QString &layerName)
{
  if (!mParentPlot)
  {
    qDebug() << Q_FUNC_INFO << "no parent QCustomPlot set";
    return false;
  }
  if (QCPLayer *layer = mParentPlot->layer(layerName))
  {
    return setLayer(layer);
  } else
  {
    qDebug() << Q_FUNC_INFO << "there is no layer with name" << layerName;
    return false;
  }
}

void QCPLayerable::setAntialiased(bool enabled)
{
  mAntialiased = enabled;
}

bool QCPLayerable::realVisibility() const
{
  return mVisible && (!mLayer || mLayer->visible()) && (!mParentLayerable || mParentLayerable.data()->realVisibility());
}

double QCPLayerable::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(pos)
  Q_UNUSED(onlySelectable)
  Q_UNUSED(details)
  return -1.0;
}

void QCPLayerable::initializeParentPlot(QCustomPlot *parentPlot)
{
  if (mParentPlot)
  {
    qDebug() << Q_FUNC_INFO << "called with mParentPlot already initialized";
    return;
  }

  if (!parentPlot)
    qDebug() << Q_FUNC_INFO << "called with parentPlot zero";

  mParentPlot = parentPlot;
  parentPlotInitialized(mParentPlot);
}

void QCPLayerable::setParentLayerable(QCPLayerable *parentLayerable)
{
  mParentLayerable = parentLayerable;
}

bool QCPLayerable::moveToLayer(QCPLayer *layer, bool prepend)
{
  if (layer && !mParentPlot)
  {
    qDebug() << Q_FUNC_INFO << "no parent QCustomPlot set";
    return false;
  }
  if (layer && layer->parentPlot() != mParentPlot)
  {
    qDebug() << Q_FUNC_INFO << "layer" << layer->name() << "is not in same QCustomPlot as this layerable";
    return false;
  }

  QCPLayer *oldLayer = mLayer;
  if (mLayer)
    mLayer->removeChild(this);
  mLayer = layer;
  if (mLayer)
    mLayer->addChild(this, prepend);
  if (mLayer != oldLayer)
    Q_EMIT layerChanged(mLayer);
  return true;
}

void QCPLayerable::applyAntialiasingHint(QCPPainter *painter, bool localAntialiased, QCP::AntialiasedElement overrideElement) const
{
  if (mParentPlot && mParentPlot->notAntialiasedElements().testFlag(overrideElement))
    painter->setAntialiasing(false);
  else if (mParentPlot && mParentPlot->antialiasedElements().testFlag(overrideElement))
    painter->setAntialiasing(true);
  else
    painter->setAntialiasing(localAntialiased);
}

void QCPLayerable::parentPlotInitialized(QCustomPlot *parentPlot)
{
   Q_UNUSED(parentPlot)
}

QCP::Interaction QCPLayerable::selectionCategory() const
{
  return QCP::iSelectOther;
}

QRect QCPLayerable::clipRect() const
{
  if (mParentPlot)
    return mParentPlot->viewport();
  else
    return QRect();
}

void QCPLayerable::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  Q_UNUSED(additive)
  Q_UNUSED(details)
  Q_UNUSED(selectionStateChanged)
}

void QCPLayerable::deselectEvent(bool *selectionStateChanged)
{
  Q_UNUSED(selectionStateChanged)
}

const double QCPRange::minRange = 1e-280;

const double QCPRange::maxRange = 1e250;

QCPRange::QCPRange() :
  lower(0),
  upper(0)
{
}

QCPRange::QCPRange(double lower, double upper) :
  lower(lower),
  upper(upper)
{
  normalize();
}

double QCPRange::size() const
{
  return upper-lower;
}

double QCPRange::center() const
{
  return (upper+lower)*0.5;
}

void QCPRange::normalize()
{
  if (lower > upper)
    qSwap(lower, upper);
}

void QCPRange::expand(const QCPRange &otherRange)
{
  if (lower > otherRange.lower)
    lower = otherRange.lower;
  if (upper < otherRange.upper)
    upper = otherRange.upper;
}

QCPRange QCPRange::expanded(const QCPRange &otherRange) const
{
  QCPRange result = *this;
  result.expand(otherRange);
  return result;
}

QCPRange QCPRange::sanitizedForLogScale() const
{
  double rangeFac = 1e-3;
  QCPRange sanitizedRange(lower, upper);
  sanitizedRange.normalize();

  if (sanitizedRange.lower == 0.0 && sanitizedRange.upper != 0.0)
  {

    if (rangeFac < sanitizedRange.upper*rangeFac)
      sanitizedRange.lower = rangeFac;
    else
      sanitizedRange.lower = sanitizedRange.upper*rangeFac;
  }
  else if (sanitizedRange.lower != 0.0 && sanitizedRange.upper == 0.0)
  {

    if (-rangeFac > sanitizedRange.lower*rangeFac)
      sanitizedRange.upper = -rangeFac;
    else
      sanitizedRange.upper = sanitizedRange.lower*rangeFac;
  } else if (sanitizedRange.lower < 0 && sanitizedRange.upper > 0)
  {

    if (-sanitizedRange.lower > sanitizedRange.upper)
    {

      if (-rangeFac > sanitizedRange.lower*rangeFac)
        sanitizedRange.upper = -rangeFac;
      else
        sanitizedRange.upper = sanitizedRange.lower*rangeFac;
    } else
    {

      if (rangeFac < sanitizedRange.upper*rangeFac)
        sanitizedRange.lower = rangeFac;
      else
        sanitizedRange.lower = sanitizedRange.upper*rangeFac;
    }
  }

  return sanitizedRange;
}

QCPRange QCPRange::sanitizedForLinScale() const
{
  QCPRange sanitizedRange(lower, upper);
  sanitizedRange.normalize();
  return sanitizedRange;
}

bool QCPRange::contains(double value) const
{
  return value >= lower && value <= upper;
}

bool QCPRange::validRange(double lower, double upper)
{

  return (lower > -maxRange &&
          upper < maxRange &&
          qAbs(lower-upper) > minRange &&
          qAbs(lower-upper) < maxRange);
}

bool QCPRange::validRange(const QCPRange &range)
{

  return (range.lower > -maxRange &&
          range.upper < maxRange &&
          qAbs(range.lower-range.upper) > minRange &&
          qAbs(range.lower-range.upper) < maxRange);
}

QCPMarginGroup::QCPMarginGroup(QCustomPlot *parentPlot) :
  QObject(parentPlot),
  mParentPlot(parentPlot)
{
  mChildren.insert(QCP::msLeft, QList<QCPLayoutElement*>());
  mChildren.insert(QCP::msRight, QList<QCPLayoutElement*>());
  mChildren.insert(QCP::msTop, QList<QCPLayoutElement*>());
  mChildren.insert(QCP::msBottom, QList<QCPLayoutElement*>());
}

QCPMarginGroup::~QCPMarginGroup()
{
  clear();
}

bool QCPMarginGroup::isEmpty() const
{
  QHashIterator<QCP::MarginSide, QList<QCPLayoutElement*> > it(mChildren);
  while (it.hasNext())
  {
    it.next();
    if (!it.value().isEmpty())
      return false;
  }
  return true;
}

void QCPMarginGroup::clear()
{

  QHashIterator<QCP::MarginSide, QList<QCPLayoutElement*> > it(mChildren);
  while (it.hasNext())
  {
    it.next();
    const QList<QCPLayoutElement*> elements = it.value();
    for (int i=elements.size()-1; i>=0; --i)
      elements.at(i)->setMarginGroup(it.key(), 0);
  }
}

int QCPMarginGroup::commonMargin(QCP::MarginSide side) const
{

  int result = 0;
  const QList<QCPLayoutElement*> elements = mChildren.value(side);
  for (int i=0; i<elements.size(); ++i)
  {
    if (!elements.at(i)->autoMargins().testFlag(side))
      continue;
    int m = qMax(elements.at(i)->calculateAutoMargin(side), QCP::getMarginValue(elements.at(i)->minimumMargins(), side));
    if (m > result)
      result = m;
  }
  return result;
}

void QCPMarginGroup::addChild(QCP::MarginSide side, QCPLayoutElement *element)
{
  if (!mChildren[side].contains(element))
    mChildren[side].append(element);
  else
    qDebug() << Q_FUNC_INFO << "element is already child of this margin group side" << reinterpret_cast<quintptr>(element);
}

void QCPMarginGroup::removeChild(QCP::MarginSide side, QCPLayoutElement *element)
{
  if (!mChildren[side].removeOne(element))
    qDebug() << Q_FUNC_INFO << "element is not child of this margin group side" << reinterpret_cast<quintptr>(element);
}

QCPLayoutElement::QCPLayoutElement(QCustomPlot *parentPlot) :
  QCPLayerable(parentPlot),
  mParentLayout(0),
  mMinimumSize(),
  mMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX),
  mRect(0, 0, 0, 0),
  mOuterRect(0, 0, 0, 0),
  mMargins(0, 0, 0, 0),
  mMinimumMargins(0, 0, 0, 0),
  mAutoMargins(QCP::msAll)
{
}

QCPLayoutElement::~QCPLayoutElement()
{
  setMarginGroup(QCP::msAll, 0);

  if (qobject_cast<QCPLayout*>(mParentLayout))
    mParentLayout->take(this);
}

void QCPLayoutElement::setOuterRect(const QRect &rect)
{
  if (mOuterRect != rect)
  {
    mOuterRect = rect;
    mRect = mOuterRect.adjusted(mMargins.left(), mMargins.top(), -mMargins.right(), -mMargins.bottom());
  }
}

void QCPLayoutElement::setMargins(const QMargins &margins)
{
  if (mMargins != margins)
  {
    mMargins = margins;
    mRect = mOuterRect.adjusted(mMargins.left(), mMargins.top(), -mMargins.right(), -mMargins.bottom());
  }
}

void QCPLayoutElement::setMinimumMargins(const QMargins &margins)
{
  if (mMinimumMargins != margins)
  {
    mMinimumMargins = margins;
  }
}

void QCPLayoutElement::setAutoMargins(QCP::MarginSides sides)
{
  mAutoMargins = sides;
}

void QCPLayoutElement::setMinimumSize(const QSize &size)
{
  if (mMinimumSize != size)
  {
    mMinimumSize = size;
    if (mParentLayout)
      mParentLayout->sizeConstraintsChanged();
  }
}

void QCPLayoutElement::setMinimumSize(int width, int height)
{
  setMinimumSize(QSize(width, height));
}

void QCPLayoutElement::setMaximumSize(const QSize &size)
{
  if (mMaximumSize != size)
  {
    mMaximumSize = size;
    if (mParentLayout)
      mParentLayout->sizeConstraintsChanged();
  }
}

void QCPLayoutElement::setMaximumSize(int width, int height)
{
  setMaximumSize(QSize(width, height));
}

void QCPLayoutElement::setMarginGroup(QCP::MarginSides sides, QCPMarginGroup *group)
{
  QVector<QCP::MarginSide> sideVector;
  if (sides.testFlag(QCP::msLeft)) sideVector.append(QCP::msLeft);
  if (sides.testFlag(QCP::msRight)) sideVector.append(QCP::msRight);
  if (sides.testFlag(QCP::msTop)) sideVector.append(QCP::msTop);
  if (sides.testFlag(QCP::msBottom)) sideVector.append(QCP::msBottom);

  for (int i=0; i<sideVector.size(); ++i)
  {
    QCP::MarginSide side = sideVector.at(i);
    if (marginGroup(side) != group)
    {
      QCPMarginGroup *oldGroup = marginGroup(side);
      if (oldGroup)
        oldGroup->removeChild(side, this);

      if (!group)
      {
        mMarginGroups.remove(side);
      } else
      {
        mMarginGroups[side] = group;
        group->addChild(side, this);
      }
    }
  }
}

void QCPLayoutElement::update(UpdatePhase phase)
{
  if (phase == upMargins)
  {
    if (mAutoMargins != QCP::msNone)
    {

      QMargins newMargins = mMargins;
      Q_FOREACH (QCP::MarginSide side, QList<QCP::MarginSide>() << QCP::msLeft << QCP::msRight << QCP::msTop << QCP::msBottom)
      {
        if (mAutoMargins.testFlag(side))
        {
          if (mMarginGroups.contains(side))
            QCP::setMarginValue(newMargins, side, mMarginGroups[side]->commonMargin(side));
          else
            QCP::setMarginValue(newMargins, side, calculateAutoMargin(side));

          if (QCP::getMarginValue(newMargins, side) < QCP::getMarginValue(mMinimumMargins, side))
            QCP::setMarginValue(newMargins, side, QCP::getMarginValue(mMinimumMargins, side));
        }
      }
      setMargins(newMargins);
    }
  }
}

QSize QCPLayoutElement::minimumSizeHint() const
{
  return mMinimumSize;
}

QSize QCPLayoutElement::maximumSizeHint() const
{
  return mMaximumSize;
}

QList<QCPLayoutElement*> QCPLayoutElement::elements(bool recursive) const
{
  Q_UNUSED(recursive)
  return QList<QCPLayoutElement*>();
}

double QCPLayoutElement::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)

  if (onlySelectable)
    return -1;

  if (QRectF(mOuterRect).contains(pos))
  {
    if (mParentPlot)
      return mParentPlot->selectionTolerance()*0.99;
    else
    {
      qDebug() << Q_FUNC_INFO << "parent plot not defined";
      return -1;
    }
  } else
    return -1;
}

void QCPLayoutElement::parentPlotInitialized(QCustomPlot *parentPlot)
{
  Q_FOREACH (QCPLayoutElement* el, elements(false))
  {
    if (!el->parentPlot())
      el->initializeParentPlot(parentPlot);
  }
}

int QCPLayoutElement::calculateAutoMargin(QCP::MarginSide side)
{
  return qMax(QCP::getMarginValue(mMargins, side), QCP::getMarginValue(mMinimumMargins, side));
}

QCPLayout::QCPLayout()
{
}

void QCPLayout::update(UpdatePhase phase)
{
  QCPLayoutElement::update(phase);

  if (phase == upLayout)
    updateLayout();

  const int elCount = elementCount();
  for (int i=0; i<elCount; ++i)
  {
    if (QCPLayoutElement *el = elementAt(i))
      el->update(phase);
  }
}

QList<QCPLayoutElement*> QCPLayout::elements(bool recursive) const
{
  const int c = elementCount();
  QList<QCPLayoutElement*> result;
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
  result.reserve(c);
#endif
  for (int i=0; i<c; ++i)
    result.append(elementAt(i));
  if (recursive)
  {
    for (int i=0; i<c; ++i)
    {
      if (result.at(i))
        result << result.at(i)->elements(recursive);
    }
  }
  return result;
}

void QCPLayout::simplify()
{
}

bool QCPLayout::removeAt(int index)
{
  if (QCPLayoutElement *el = takeAt(index))
  {
    delete el;
    return true;
  } else
    return false;
}

bool QCPLayout::remove(QCPLayoutElement *element)
{
  if (take(element))
  {
    delete element;
    return true;
  } else
    return false;
}

void QCPLayout::clear()
{
  for (int i=elementCount()-1; i>=0; --i)
  {
    if (elementAt(i))
      removeAt(i);
  }
  simplify();
}

void QCPLayout::sizeConstraintsChanged() const
{
  if (QWidget *w = qobject_cast<QWidget*>(parent()))
    w->updateGeometry();
  else if (QCPLayout *l = qobject_cast<QCPLayout*>(parent()))
    l->sizeConstraintsChanged();
}

void QCPLayout::updateLayout()
{
}

void QCPLayout::adoptElement(QCPLayoutElement *el)
{
  if (el)
  {
    el->mParentLayout = this;
    el->setParentLayerable(this);
    el->setParent(this);
    if (!el->parentPlot())
      el->initializeParentPlot(mParentPlot);
  } else
    qDebug() << Q_FUNC_INFO << "Null element passed";
}

void QCPLayout::releaseElement(QCPLayoutElement *el)
{
  if (el)
  {
    el->mParentLayout = 0;
    el->setParentLayerable(0);
    el->setParent(mParentPlot);

  } else
    qDebug() << Q_FUNC_INFO << "Null element passed";
}

QVector<int> QCPLayout::getSectionSizes(QVector<int> maxSizes, QVector<int> minSizes, QVector<double> stretchFactors, int totalSize) const
{
  if (maxSizes.size() != minSizes.size() || minSizes.size() != stretchFactors.size())
  {
    qDebug() << Q_FUNC_INFO << "Passed vector sizes aren't equal:" << maxSizes << minSizes << stretchFactors;
    return QVector<int>();
  }
  if (stretchFactors.isEmpty())
    return QVector<int>();
  int sectionCount = stretchFactors.size();
  QVector<double> sectionSizes(sectionCount);

  int minSizeSum = 0;
  for (int i=0; i<sectionCount; ++i)
    minSizeSum += minSizes.at(i);
  if (totalSize < minSizeSum)
  {

    for (int i=0; i<sectionCount; ++i)
    {
      stretchFactors[i] = minSizes.at(i);
      minSizes[i] = 0;
    }
  }

  QList<int> minimumLockedSections;
  QList<int> unfinishedSections;
  for (int i=0; i<sectionCount; ++i)
    unfinishedSections.append(i);
  double freeSize = totalSize;

  int outerIterations = 0;
  while (!unfinishedSections.isEmpty() && outerIterations < sectionCount*2)
  {
    ++outerIterations;
    int innerIterations = 0;
    while (!unfinishedSections.isEmpty() && innerIterations < sectionCount*2)
    {
      ++innerIterations;

      int nextId = -1;
      double nextMax = 1e12;
      for (int i=0; i<unfinishedSections.size(); ++i)
      {
        int secId = unfinishedSections.at(i);
        double hitsMaxAt = (maxSizes.at(secId)-sectionSizes.at(secId))/stretchFactors.at(secId);
        if (hitsMaxAt < nextMax)
        {
          nextMax = hitsMaxAt;
          nextId = secId;
        }
      }

      double stretchFactorSum = 0;
      for (int i=0; i<unfinishedSections.size(); ++i)
        stretchFactorSum += stretchFactors.at(unfinishedSections.at(i));
      double nextMaxLimit = freeSize/stretchFactorSum;
      if (nextMax < nextMaxLimit)
      {
        for (int i=0; i<unfinishedSections.size(); ++i)
        {
          sectionSizes[unfinishedSections.at(i)] += nextMax*stretchFactors.at(unfinishedSections.at(i));
          freeSize -= nextMax*stretchFactors.at(unfinishedSections.at(i));
        }
        unfinishedSections.removeOne(nextId);
      } else
      {
        for (int i=0; i<unfinishedSections.size(); ++i)
          sectionSizes[unfinishedSections.at(i)] += nextMaxLimit*stretchFactors.at(unfinishedSections.at(i));
        unfinishedSections.clear();
      }
    }
    if (innerIterations == sectionCount*2)
      qDebug() << Q_FUNC_INFO << "Exceeded maximum expected inner iteration count, layouting aborted. Input was:" << maxSizes << minSizes << stretchFactors << totalSize;

    bool foundMinimumViolation = false;
    for (int i=0; i<sectionSizes.size(); ++i)
    {
      if (minimumLockedSections.contains(i))
        continue;
      if (sectionSizes.at(i) < minSizes.at(i))
      {
        sectionSizes[i] = minSizes.at(i);
        foundMinimumViolation = true;
        minimumLockedSections.append(i);
      }
    }
    if (foundMinimumViolation)
    {
      freeSize = totalSize;
      for (int i=0; i<sectionCount; ++i)
      {
        if (!minimumLockedSections.contains(i))
          unfinishedSections.append(i);
        else
          freeSize -= sectionSizes.at(i);
      }

      for (int i=0; i<unfinishedSections.size(); ++i)
        sectionSizes[unfinishedSections.at(i)] = 0;
    }
  }
  if (outerIterations == sectionCount*2)
    qDebug() << Q_FUNC_INFO << "Exceeded maximum expected outer iteration count, layouting aborted. Input was:" << maxSizes << minSizes << stretchFactors << totalSize;

  QVector<int> result(sectionCount);
  for (int i=0; i<sectionCount; ++i)
    result[i] = qRound(sectionSizes.at(i));
  return result;
}

QCPLayoutGrid::QCPLayoutGrid() :
  mColumnSpacing(5),
  mRowSpacing(5)
{
}

QCPLayoutGrid::~QCPLayoutGrid()
{

  clear();
}

QCPLayoutElement *QCPLayoutGrid::element(int row, int column) const
{
  if (row >= 0 && row < mElements.size())
  {
    if (column >= 0 && column < mElements.first().size())
    {
      if (QCPLayoutElement *result = mElements.at(row).at(column))
        return result;
      else
        qDebug() << Q_FUNC_INFO << "Requested cell is empty. Row:" << row << "Column:" << column;
    } else
      qDebug() << Q_FUNC_INFO << "Invalid column. Row:" << row << "Column:" << column;
  } else
    qDebug() << Q_FUNC_INFO << "Invalid row. Row:" << row << "Column:" << column;
  return 0;
}

int QCPLayoutGrid::rowCount() const
{
  return mElements.size();
}

int QCPLayoutGrid::columnCount() const
{
  if (mElements.size() > 0)
    return mElements.first().size();
  else
    return 0;
}

bool QCPLayoutGrid::addElement(int row, int column, QCPLayoutElement *element)
{
  if (element)
  {
    if (!hasElement(row, column))
    {
      if (element->layout())
        element->layout()->take(element);
      expandTo(row+1, column+1);
      mElements[row][column] = element;
      adoptElement(element);
      return true;
    } else
      qDebug() << Q_FUNC_INFO << "There is already an element in the specified row/column:" << row << column;
  } else
    qDebug() << Q_FUNC_INFO << "Can't add null element to row/column:" << row << column;
  return false;
}

bool QCPLayoutGrid::hasElement(int row, int column)
{
  if (row >= 0 && row < rowCount() && column >= 0 && column < columnCount())
    return mElements.at(row).at(column);
  else
    return false;
}

void QCPLayoutGrid::setColumnStretchFactor(int column, double factor)
{
  if (column >= 0 && column < columnCount())
  {
    if (factor > 0)
      mColumnStretchFactors[column] = factor;
    else
      qDebug() << Q_FUNC_INFO << "Invalid stretch factor, must be positive:" << factor;
  } else
    qDebug() << Q_FUNC_INFO << "Invalid column:" << column;
}

void QCPLayoutGrid::setColumnStretchFactors(const QList<double> &factors)
{
  if (factors.size() == mColumnStretchFactors.size())
  {
    mColumnStretchFactors = factors;
    for (int i=0; i<mColumnStretchFactors.size(); ++i)
    {
      if (mColumnStretchFactors.at(i) <= 0)
      {
        qDebug() << Q_FUNC_INFO << "Invalid stretch factor, must be positive:" << mColumnStretchFactors.at(i);
        mColumnStretchFactors[i] = 1;
      }
    }
  } else
    qDebug() << Q_FUNC_INFO << "Column count not equal to passed stretch factor count:" << factors;
}

void QCPLayoutGrid::setRowStretchFactor(int row, double factor)
{
  if (row >= 0 && row < rowCount())
  {
    if (factor > 0)
      mRowStretchFactors[row] = factor;
    else
      qDebug() << Q_FUNC_INFO << "Invalid stretch factor, must be positive:" << factor;
  } else
    qDebug() << Q_FUNC_INFO << "Invalid row:" << row;
}

void QCPLayoutGrid::setRowStretchFactors(const QList<double> &factors)
{
  if (factors.size() == mRowStretchFactors.size())
  {
    mRowStretchFactors = factors;
    for (int i=0; i<mRowStretchFactors.size(); ++i)
    {
      if (mRowStretchFactors.at(i) <= 0)
      {
        qDebug() << Q_FUNC_INFO << "Invalid stretch factor, must be positive:" << mRowStretchFactors.at(i);
        mRowStretchFactors[i] = 1;
      }
    }
  } else
    qDebug() << Q_FUNC_INFO << "Row count not equal to passed stretch factor count:" << factors;
}

void QCPLayoutGrid::setColumnSpacing(int pixels)
{
  mColumnSpacing = pixels;
}

void QCPLayoutGrid::setRowSpacing(int pixels)
{
  mRowSpacing = pixels;
}

void QCPLayoutGrid::expandTo(int newRowCount, int newColumnCount)
{

  while (rowCount() < newRowCount)
  {
    mElements.append(QList<QCPLayoutElement*>());
    mRowStretchFactors.append(1);
  }

  int newColCount = qMax(columnCount(), newColumnCount);
  for (int i=0; i<rowCount(); ++i)
  {
    while (mElements.at(i).size() < newColCount)
      mElements[i].append(0);
  }
  while (mColumnStretchFactors.size() < newColCount)
    mColumnStretchFactors.append(1);
}

void QCPLayoutGrid::insertRow(int newIndex)
{
  if (mElements.isEmpty() || mElements.first().isEmpty())
  {
    expandTo(1, 1);
    return;
  }

  if (newIndex < 0)
    newIndex = 0;
  if (newIndex > rowCount())
    newIndex = rowCount();

  mRowStretchFactors.insert(newIndex, 1);
  QList<QCPLayoutElement*> newRow;
  for (int col=0; col<columnCount(); ++col)
    newRow.append((QCPLayoutElement*)0);
  mElements.insert(newIndex, newRow);
}

void QCPLayoutGrid::insertColumn(int newIndex)
{
  if (mElements.isEmpty() || mElements.first().isEmpty())
  {
    expandTo(1, 1);
    return;
  }

  if (newIndex < 0)
    newIndex = 0;
  if (newIndex > columnCount())
    newIndex = columnCount();

  mColumnStretchFactors.insert(newIndex, 1);
  for (int row=0; row<rowCount(); ++row)
    mElements[row].insert(newIndex, (QCPLayoutElement*)0);
}

void QCPLayoutGrid::updateLayout()
{
  QVector<int> minColWidths, minRowHeights, maxColWidths, maxRowHeights;
  getMinimumRowColSizes(&minColWidths, &minRowHeights);
  getMaximumRowColSizes(&maxColWidths, &maxRowHeights);

  int totalRowSpacing = (rowCount()-1) * mRowSpacing;
  int totalColSpacing = (columnCount()-1) * mColumnSpacing;
  QVector<int> colWidths = getSectionSizes(maxColWidths, minColWidths, mColumnStretchFactors.toVector(), mRect.width()-totalColSpacing);
  QVector<int> rowHeights = getSectionSizes(maxRowHeights, minRowHeights, mRowStretchFactors.toVector(), mRect.height()-totalRowSpacing);

  int yOffset = mRect.top();
  for (int row=0; row<rowCount(); ++row)
  {
    if (row > 0)
      yOffset += rowHeights.at(row-1)+mRowSpacing;
    int xOffset = mRect.left();
    for (int col=0; col<columnCount(); ++col)
    {
      if (col > 0)
        xOffset += colWidths.at(col-1)+mColumnSpacing;
      if (mElements.at(row).at(col))
        mElements.at(row).at(col)->setOuterRect(QRect(xOffset, yOffset, colWidths.at(col), rowHeights.at(row)));
    }
  }
}

int QCPLayoutGrid::elementCount() const
{
  return rowCount()*columnCount();
}

QCPLayoutElement *QCPLayoutGrid::elementAt(int index) const
{
  if (index >= 0 && index < elementCount())
    return mElements.at(index / columnCount()).at(index % columnCount());
  else
    return 0;
}

QCPLayoutElement *QCPLayoutGrid::takeAt(int index)
{
  if (QCPLayoutElement *el = elementAt(index))
  {
    releaseElement(el);
    mElements[index / columnCount()][index % columnCount()] = 0;
    return el;
  } else
  {
    qDebug() << Q_FUNC_INFO << "Attempt to take invalid index:" << index;
    return 0;
  }
}

bool QCPLayoutGrid::take(QCPLayoutElement *element)
{
  if (element)
  {
    for (int i=0; i<elementCount(); ++i)
    {
      if (elementAt(i) == element)
      {
        takeAt(i);
        return true;
      }
    }
    qDebug() << Q_FUNC_INFO << "Element not in this layout, couldn't take";
  } else
    qDebug() << Q_FUNC_INFO << "Can't take null element";
  return false;
}

QList<QCPLayoutElement*> QCPLayoutGrid::elements(bool recursive) const
{
  QList<QCPLayoutElement*> result;
  int colC = columnCount();
  int rowC = rowCount();
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
  result.reserve(colC*rowC);
#endif
  for (int row=0; row<rowC; ++row)
  {
    for (int col=0; col<colC; ++col)
    {
      result.append(mElements.at(row).at(col));
    }
  }
  if (recursive)
  {
    int c = result.size();
    for (int i=0; i<c; ++i)
    {
      if (result.at(i))
        result << result.at(i)->elements(recursive);
    }
  }
  return result;
}

void QCPLayoutGrid::simplify()
{

  for (int row=rowCount()-1; row>=0; --row)
  {
    bool hasElements = false;
    for (int col=0; col<columnCount(); ++col)
    {
      if (mElements.at(row).at(col))
      {
        hasElements = true;
        break;
      }
    }
    if (!hasElements)
    {
      mRowStretchFactors.removeAt(row);
      mElements.removeAt(row);
      if (mElements.isEmpty())
        mColumnStretchFactors.clear();
    }
  }

  for (int col=columnCount()-1; col>=0; --col)
  {
    bool hasElements = false;
    for (int row=0; row<rowCount(); ++row)
    {
      if (mElements.at(row).at(col))
      {
        hasElements = true;
        break;
      }
    }
    if (!hasElements)
    {
      mColumnStretchFactors.removeAt(col);
      for (int row=0; row<rowCount(); ++row)
        mElements[row].removeAt(col);
    }
  }
}

QSize QCPLayoutGrid::minimumSizeHint() const
{
  QVector<int> minColWidths, minRowHeights;
  getMinimumRowColSizes(&minColWidths, &minRowHeights);
  QSize result(0, 0);
  for (int i=0; i<minColWidths.size(); ++i)
    result.rwidth() += minColWidths.at(i);
  for (int i=0; i<minRowHeights.size(); ++i)
    result.rheight() += minRowHeights.at(i);
  result.rwidth() += qMax(0, columnCount()-1) * mColumnSpacing + mMargins.left() + mMargins.right();
  result.rheight() += qMax(0, rowCount()-1) * mRowSpacing + mMargins.top() + mMargins.bottom();
  return result;
}

QSize QCPLayoutGrid::maximumSizeHint() const
{
  QVector<int> maxColWidths, maxRowHeights;
  getMaximumRowColSizes(&maxColWidths, &maxRowHeights);

  QSize result(0, 0);
  for (int i=0; i<maxColWidths.size(); ++i)
    result.setWidth(qMin(result.width()+maxColWidths.at(i), QWIDGETSIZE_MAX));
  for (int i=0; i<maxRowHeights.size(); ++i)
    result.setHeight(qMin(result.height()+maxRowHeights.at(i), QWIDGETSIZE_MAX));
  result.rwidth() += qMax(0, columnCount()-1) * mColumnSpacing + mMargins.left() + mMargins.right();
  result.rheight() += qMax(0, rowCount()-1) * mRowSpacing + mMargins.top() + mMargins.bottom();
  return result;
}

void QCPLayoutGrid::getMinimumRowColSizes(QVector<int> *minColWidths, QVector<int> *minRowHeights) const
{
  *minColWidths = QVector<int>(columnCount(), 0);
  *minRowHeights = QVector<int>(rowCount(), 0);
  for (int row=0; row<rowCount(); ++row)
  {
    for (int col=0; col<columnCount(); ++col)
    {
      if (mElements.at(row).at(col))
      {
        QSize minHint = mElements.at(row).at(col)->minimumSizeHint();
        QSize min = mElements.at(row).at(col)->minimumSize();
        QSize final(min.width() > 0 ? min.width() : minHint.width(), min.height() > 0 ? min.height() : minHint.height());
        if (minColWidths->at(col) < final.width())
          (*minColWidths)[col] = final.width();
        if (minRowHeights->at(row) < final.height())
          (*minRowHeights)[row] = final.height();
      }
    }
  }
}

void QCPLayoutGrid::getMaximumRowColSizes(QVector<int> *maxColWidths, QVector<int> *maxRowHeights) const
{
  *maxColWidths = QVector<int>(columnCount(), QWIDGETSIZE_MAX);
  *maxRowHeights = QVector<int>(rowCount(), QWIDGETSIZE_MAX);
  for (int row=0; row<rowCount(); ++row)
  {
    for (int col=0; col<columnCount(); ++col)
    {
      if (mElements.at(row).at(col))
      {
        QSize maxHint = mElements.at(row).at(col)->maximumSizeHint();
        QSize max = mElements.at(row).at(col)->maximumSize();
        QSize final(max.width() < QWIDGETSIZE_MAX ? max.width() : maxHint.width(), max.height() < QWIDGETSIZE_MAX ? max.height() : maxHint.height());
        if (maxColWidths->at(col) > final.width())
          (*maxColWidths)[col] = final.width();
        if (maxRowHeights->at(row) > final.height())
          (*maxRowHeights)[row] = final.height();
      }
    }
  }
}

QCPLayoutInset::QCPLayoutInset()
{
}

QCPLayoutInset::~QCPLayoutInset()
{

  clear();
}

QCPLayoutInset::InsetPlacement QCPLayoutInset::insetPlacement(int index) const
{
  if (elementAt(index))
    return mInsetPlacement.at(index);
  else
  {
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
    return ipFree;
  }
}

Qt::Alignment QCPLayoutInset::insetAlignment(int index) const
{
  if (elementAt(index))
    return mInsetAlignment.at(index);
  else
  {
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
    return 0;
  }
}

QRectF QCPLayoutInset::insetRect(int index) const
{
  if (elementAt(index))
    return mInsetRect.at(index);
  else
  {
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
    return QRectF();
  }
}

void QCPLayoutInset::setInsetPlacement(int index, QCPLayoutInset::InsetPlacement placement)
{
  if (elementAt(index))
    mInsetPlacement[index] = placement;
  else
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
}

void QCPLayoutInset::setInsetAlignment(int index, Qt::Alignment alignment)
{
  if (elementAt(index))
    mInsetAlignment[index] = alignment;
  else
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
}

void QCPLayoutInset::setInsetRect(int index, const QRectF &rect)
{
  if (elementAt(index))
    mInsetRect[index] = rect;
  else
    qDebug() << Q_FUNC_INFO << "Invalid element index:" << index;
}

void QCPLayoutInset::updateLayout()
{
  for (int i=0; i<mElements.size(); ++i)
  {
    QRect insetRect;
    QSize finalMinSize, finalMaxSize;
    QSize minSizeHint = mElements.at(i)->minimumSizeHint();
    QSize maxSizeHint = mElements.at(i)->maximumSizeHint();
    finalMinSize.setWidth(mElements.at(i)->minimumSize().width() > 0 ? mElements.at(i)->minimumSize().width() : minSizeHint.width());
    finalMinSize.setHeight(mElements.at(i)->minimumSize().height() > 0 ? mElements.at(i)->minimumSize().height() : minSizeHint.height());
    finalMaxSize.setWidth(mElements.at(i)->maximumSize().width() < QWIDGETSIZE_MAX ? mElements.at(i)->maximumSize().width() : maxSizeHint.width());
    finalMaxSize.setHeight(mElements.at(i)->maximumSize().height() < QWIDGETSIZE_MAX ? mElements.at(i)->maximumSize().height() : maxSizeHint.height());
    if (mInsetPlacement.at(i) == ipFree)
    {
      insetRect = QRect(rect().x()+rect().width()*mInsetRect.at(i).x(),
                        rect().y()+rect().height()*mInsetRect.at(i).y(),
                        rect().width()*mInsetRect.at(i).width(),
                        rect().height()*mInsetRect.at(i).height());
      if (insetRect.size().width() < finalMinSize.width())
        insetRect.setWidth(finalMinSize.width());
      if (insetRect.size().height() < finalMinSize.height())
        insetRect.setHeight(finalMinSize.height());
      if (insetRect.size().width() > finalMaxSize.width())
        insetRect.setWidth(finalMaxSize.width());
      if (insetRect.size().height() > finalMaxSize.height())
        insetRect.setHeight(finalMaxSize.height());
    } else if (mInsetPlacement.at(i) == ipBorderAligned)
    {
      insetRect.setSize(finalMinSize);
      Qt::Alignment al = mInsetAlignment.at(i);
      if (al.testFlag(Qt::AlignLeft)) insetRect.moveLeft(rect().x());
      else if (al.testFlag(Qt::AlignRight)) insetRect.moveRight(rect().x()+rect().width());
      else insetRect.moveLeft(rect().x()+rect().width()*0.5-finalMinSize.width()*0.5);
      if (al.testFlag(Qt::AlignTop)) insetRect.moveTop(rect().y());
      else if (al.testFlag(Qt::AlignBottom)) insetRect.moveBottom(rect().y()+rect().height());
      else insetRect.moveTop(rect().y()+rect().height()*0.5-finalMinSize.height()*0.5);
    }
    mElements.at(i)->setOuterRect(insetRect);
  }
}

int QCPLayoutInset::elementCount() const
{
  return mElements.size();
}

QCPLayoutElement *QCPLayoutInset::elementAt(int index) const
{
  if (index >= 0 && index < mElements.size())
    return mElements.at(index);
  else
    return 0;
}

QCPLayoutElement *QCPLayoutInset::takeAt(int index)
{
  if (QCPLayoutElement *el = elementAt(index))
  {
    releaseElement(el);
    mElements.removeAt(index);
    mInsetPlacement.removeAt(index);
    mInsetAlignment.removeAt(index);
    mInsetRect.removeAt(index);
    return el;
  } else
  {
    qDebug() << Q_FUNC_INFO << "Attempt to take invalid index:" << index;
    return 0;
  }
}

bool QCPLayoutInset::take(QCPLayoutElement *element)
{
  if (element)
  {
    for (int i=0; i<elementCount(); ++i)
    {
      if (elementAt(i) == element)
      {
        takeAt(i);
        return true;
      }
    }
    qDebug() << Q_FUNC_INFO << "Element not in this layout, couldn't take";
  } else
    qDebug() << Q_FUNC_INFO << "Can't take null element";
  return false;
}

double QCPLayoutInset::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable)
    return -1;

  for (int i=0; i<mElements.size(); ++i)
  {

    if (mElements.at(i)->realVisibility() && mElements.at(i)->selectTest(pos, onlySelectable) >= 0)
      return mParentPlot->selectionTolerance()*0.99;
  }
  return -1;
}

void QCPLayoutInset::addElement(QCPLayoutElement *element, Qt::Alignment alignment)
{
  if (element)
  {
    if (element->layout())
      element->layout()->take(element);
    mElements.append(element);
    mInsetPlacement.append(ipBorderAligned);
    mInsetAlignment.append(alignment);
    mInsetRect.append(QRectF(0.6, 0.6, 0.4, 0.4));
    adoptElement(element);
  } else
    qDebug() << Q_FUNC_INFO << "Can't add null element";
}

void QCPLayoutInset::addElement(QCPLayoutElement *element, const QRectF &rect)
{
  if (element)
  {
    if (element->layout())
      element->layout()->take(element);
    mElements.append(element);
    mInsetPlacement.append(ipFree);
    mInsetAlignment.append(Qt::AlignRight|Qt::AlignTop);
    mInsetRect.append(rect);
    adoptElement(element);
  } else
    qDebug() << Q_FUNC_INFO << "Can't add null element";
}

QCPLineEnding::QCPLineEnding() :
  mStyle(esNone),
  mWidth(8),
  mLength(10),
  mInverted(false)
{
}

QCPLineEnding::QCPLineEnding(QCPLineEnding::EndingStyle style, double width, double length, bool inverted) :
  mStyle(style),
  mWidth(width),
  mLength(length),
  mInverted(inverted)
{
}

void QCPLineEnding::setStyle(QCPLineEnding::EndingStyle style)
{
  mStyle = style;
}

void QCPLineEnding::setWidth(double width)
{
  mWidth = width;
}

void QCPLineEnding::setLength(double length)
{
  mLength = length;
}

void QCPLineEnding::setInverted(bool inverted)
{
  mInverted = inverted;
}

double QCPLineEnding::boundingDistance() const
{
  switch (mStyle)
  {
    case esNone:
      return 0;

    case esFlatArrow:
    case esSpikeArrow:
    case esLineArrow:
    case esSkewedBar:
      return qSqrt(mWidth*mWidth+mLength*mLength);

    case esDisc:
    case esSquare:
    case esDiamond:
    case esBar:
    case esHalfBar:
      return mWidth*1.42;

  }
  return 0;
}

double QCPLineEnding::realLength() const
{
  switch (mStyle)
  {
    case esNone:
    case esLineArrow:
    case esSkewedBar:
    case esBar:
    case esHalfBar:
      return 0;

    case esFlatArrow:
      return mLength;

    case esDisc:
    case esSquare:
    case esDiamond:
      return mWidth*0.5;

    case esSpikeArrow:
      return mLength*0.8;
  }
  return 0;
}

void QCPLineEnding::draw(QCPPainter *painter, const QVector2D &pos, const QVector2D &dir) const
{
  if (mStyle == esNone)
    return;

  QVector2D lengthVec(dir.normalized());
  if (lengthVec.isNull())
    lengthVec = QVector2D(1, 0);
  QVector2D widthVec(-lengthVec.y(), lengthVec.x());
  lengthVec *= (float)(mLength*(mInverted ? -1 : 1));
  widthVec *= (float)(mWidth*0.5*(mInverted ? -1 : 1));

  QPen penBackup = painter->pen();
  QBrush brushBackup = painter->brush();
  QPen miterPen = penBackup;
  miterPen.setJoinStyle(Qt::MiterJoin);
  QBrush brush(painter->pen().color(), Qt::SolidPattern);
  switch (mStyle)
  {
    case esNone: break;
    case esFlatArrow:
    {
      QPointF points[3] = {pos.toPointF(),
                           (pos-lengthVec+widthVec).toPointF(),
                           (pos-lengthVec-widthVec).toPointF()
                          };
      painter->setPen(miterPen);
      painter->setBrush(brush);
      painter->drawConvexPolygon(points, 3);
      painter->setBrush(brushBackup);
      painter->setPen(penBackup);
      break;
    }
    case esSpikeArrow:
    {
      QPointF points[4] = {pos.toPointF(),
                           (pos-lengthVec+widthVec).toPointF(),
                           (pos-lengthVec*0.8f).toPointF(),
                           (pos-lengthVec-widthVec).toPointF()
                          };
      painter->setPen(miterPen);
      painter->setBrush(brush);
      painter->drawConvexPolygon(points, 4);
      painter->setBrush(brushBackup);
      painter->setPen(penBackup);
      break;
    }
    case esLineArrow:
    {
      QPointF points[3] = {(pos-lengthVec+widthVec).toPointF(),
                           pos.toPointF(),
                           (pos-lengthVec-widthVec).toPointF()
                          };
      painter->setPen(miterPen);
      painter->drawPolyline(points, 3);
      painter->setPen(penBackup);
      break;
    }
    case esDisc:
    {
      painter->setBrush(brush);
      painter->drawEllipse(pos.toPointF(),  mWidth*0.5, mWidth*0.5);
      painter->setBrush(brushBackup);
      break;
    }
    case esSquare:
    {
      QVector2D widthVecPerp(-widthVec.y(), widthVec.x());
      QPointF points[4] = {(pos-widthVecPerp+widthVec).toPointF(),
                           (pos-widthVecPerp-widthVec).toPointF(),
                           (pos+widthVecPerp-widthVec).toPointF(),
                           (pos+widthVecPerp+widthVec).toPointF()
                          };
      painter->setPen(miterPen);
      painter->setBrush(brush);
      painter->drawConvexPolygon(points, 4);
      painter->setBrush(brushBackup);
      painter->setPen(penBackup);
      break;
    }
    case esDiamond:
    {
      QVector2D widthVecPerp(-widthVec.y(), widthVec.x());
      QPointF points[4] = {(pos-widthVecPerp).toPointF(),
                           (pos-widthVec).toPointF(),
                           (pos+widthVecPerp).toPointF(),
                           (pos+widthVec).toPointF()
                          };
      painter->setPen(miterPen);
      painter->setBrush(brush);
      painter->drawConvexPolygon(points, 4);
      painter->setBrush(brushBackup);
      painter->setPen(penBackup);
      break;
    }
    case esBar:
    {
      painter->drawLine((pos+widthVec).toPointF(), (pos-widthVec).toPointF());
      break;
    }
    case esHalfBar:
    {
      painter->drawLine((pos+widthVec).toPointF(), pos.toPointF());
      break;
    }
    case esSkewedBar:
    {
      if (qFuzzyIsNull(painter->pen().widthF()) && !painter->modes().testFlag(QCPPainter::pmNonCosmetic))
      {

        painter->drawLine((pos+widthVec+lengthVec*0.2f*(mInverted?-1:1)).toPointF(),
                          (pos-widthVec-lengthVec*0.2f*(mInverted?-1:1)).toPointF());
      } else
      {

        painter->drawLine((pos+widthVec+lengthVec*0.2f*(mInverted?-1:1)+dir.normalized()*qMax(1.0f, (float)painter->pen().widthF())*0.5f).toPointF(),
                          (pos-widthVec-lengthVec*0.2f*(mInverted?-1:1)+dir.normalized()*qMax(1.0f, (float)painter->pen().widthF())*0.5f).toPointF());
      }
      break;
    }
  }
}

void QCPLineEnding::draw(QCPPainter *painter, const QVector2D &pos, double angle) const
{
  draw(painter, pos, QVector2D(qCos(angle), qSin(angle)));
}

QCPGrid::QCPGrid(QCPAxis *parentAxis) :
  QCPLayerable(parentAxis->parentPlot(), "", parentAxis),
  mParentAxis(parentAxis)
{

  setParent(parentAxis);
  setPen(QPen(QColor(200,200,200), 0, Qt::DotLine));
  setSubGridPen(QPen(QColor(220,220,220), 0, Qt::DotLine));
  setZeroLinePen(QPen(QColor(200,200,200), 0, Qt::SolidLine));
  setSubGridVisible(false);
  setAntialiased(false);
  setAntialiasedSubGrid(false);
  setAntialiasedZeroLine(false);
}

void QCPGrid::setSubGridVisible(bool visible)
{
  mSubGridVisible = visible;
}

void QCPGrid::setAntialiasedSubGrid(bool enabled)
{
  mAntialiasedSubGrid = enabled;
}

void QCPGrid::setAntialiasedZeroLine(bool enabled)
{
  mAntialiasedZeroLine = enabled;
}

void QCPGrid::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPGrid::setSubGridPen(const QPen &pen)
{
  mSubGridPen = pen;
}

void QCPGrid::setZeroLinePen(const QPen &pen)
{
  mZeroLinePen = pen;
}

void QCPGrid::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeGrid);
}

void QCPGrid::draw(QCPPainter *painter)
{
  if (!mParentAxis) { qDebug() << Q_FUNC_INFO << "invalid parent axis"; return; }

  if (mSubGridVisible)
    drawSubGridLines(painter);
  drawGridLines(painter);
}

void QCPGrid::drawGridLines(QCPPainter *painter) const
{
  if (!mParentAxis) { qDebug() << Q_FUNC_INFO << "invalid parent axis"; return; }

  int lowTick = mParentAxis->mLowestVisibleTick;
  int highTick = mParentAxis->mHighestVisibleTick;
  double t;
  if (mParentAxis->orientation() == Qt::Horizontal)
  {

    int zeroLineIndex = -1;
    if (mZeroLinePen.style() != Qt::NoPen && mParentAxis->mRange.lower < 0 && mParentAxis->mRange.upper > 0)
    {
      applyAntialiasingHint(painter, mAntialiasedZeroLine, QCP::aeZeroLine);
      painter->setPen(mZeroLinePen);
      double epsilon = mParentAxis->range().size()*1E-6;
      for (int i=lowTick; i <= highTick; ++i)
      {
        if (qAbs(mParentAxis->mTickVector.at(i)) < epsilon)
        {
          zeroLineIndex = i;
          t = mParentAxis->coordToPixel(mParentAxis->mTickVector.at(i));
          painter->drawLine(QLineF(t, mParentAxis->mAxisRect->bottom(), t, mParentAxis->mAxisRect->top()));
          break;
        }
      }
    }

    applyDefaultAntialiasingHint(painter);
    painter->setPen(mPen);
    for (int i=lowTick; i <= highTick; ++i)
    {
      if (i == zeroLineIndex) continue;
      t = mParentAxis->coordToPixel(mParentAxis->mTickVector.at(i));
      painter->drawLine(QLineF(t, mParentAxis->mAxisRect->bottom(), t, mParentAxis->mAxisRect->top()));
    }
  } else
  {

    int zeroLineIndex = -1;
    if (mZeroLinePen.style() != Qt::NoPen && mParentAxis->mRange.lower < 0 && mParentAxis->mRange.upper > 0)
    {
      applyAntialiasingHint(painter, mAntialiasedZeroLine, QCP::aeZeroLine);
      painter->setPen(mZeroLinePen);
      double epsilon = mParentAxis->mRange.size()*1E-6;
      for (int i=lowTick; i <= highTick; ++i)
      {
        if (qAbs(mParentAxis->mTickVector.at(i)) < epsilon)
        {
          zeroLineIndex = i;
          t = mParentAxis->coordToPixel(mParentAxis->mTickVector.at(i));
          painter->drawLine(QLineF(mParentAxis->mAxisRect->left(), t, mParentAxis->mAxisRect->right(), t));
          break;
        }
      }
    }

    applyDefaultAntialiasingHint(painter);
    painter->setPen(mPen);
    for (int i=lowTick; i <= highTick; ++i)
    {
      if (i == zeroLineIndex) continue;
      t = mParentAxis->coordToPixel(mParentAxis->mTickVector.at(i));
      painter->drawLine(QLineF(mParentAxis->mAxisRect->left(), t, mParentAxis->mAxisRect->right(), t));
    }
  }
}

void QCPGrid::drawSubGridLines(QCPPainter *painter) const
{
  if (!mParentAxis) { qDebug() << Q_FUNC_INFO << "invalid parent axis"; return; }

  applyAntialiasingHint(painter, mAntialiasedSubGrid, QCP::aeSubGrid);
  double t;
  painter->setPen(mSubGridPen);
  if (mParentAxis->orientation() == Qt::Horizontal)
  {
    for (int i=0; i<mParentAxis->mSubTickVector.size(); ++i)
    {
      t = mParentAxis->coordToPixel(mParentAxis->mSubTickVector.at(i));
      painter->drawLine(QLineF(t, mParentAxis->mAxisRect->bottom(), t, mParentAxis->mAxisRect->top()));
    }
  } else
  {
    for (int i=0; i<mParentAxis->mSubTickVector.size(); ++i)
    {
      t = mParentAxis->coordToPixel(mParentAxis->mSubTickVector.at(i));
      painter->drawLine(QLineF(mParentAxis->mAxisRect->left(), t, mParentAxis->mAxisRect->right(), t));
    }
  }
}

QCPAxis::QCPAxis(QCPAxisRect *parent, AxisType type) :
  QCPLayerable(parent->parentPlot(), "", parent),

  mAxisType(type),
  mAxisRect(parent),
  mPadding(5),
  mOrientation(orientation(type)),
  mSelectableParts(spAxis | spTickLabels | spAxisLabel),
  mSelectedParts(spNone),
  mBasePen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  mSelectedBasePen(QPen(Qt::blue, 2)),

  mLabel(""),
  mLabelFont(mParentPlot->font()),
  mSelectedLabelFont(QFont(mLabelFont.family(), mLabelFont.pointSize(), QFont::Bold)),
  mLabelColor(Qt::black),
  mSelectedLabelColor(Qt::blue),

  mTickLabels(true),
  mAutoTickLabels(true),
  mTickLabelType(ltNumber),
  mTickLabelFont(mParentPlot->font()),
  mSelectedTickLabelFont(QFont(mTickLabelFont.family(), mTickLabelFont.pointSize(), QFont::Bold)),
  mTickLabelColor(Qt::black),
  mSelectedTickLabelColor(Qt::blue),
  mDateTimeFormat("hh:mm:ss\ndd.MM.yy"),
  mDateTimeSpec(Qt::LocalTime),
  mNumberPrecision(6),
  mNumberFormatChar('g'),
  mNumberBeautifulPowers(true),

  mTicks(true),
  mTickStep(1),
  mSubTickCount(4),
  mAutoTickCount(6),
  mAutoTicks(true),
  mAutoTickStep(true),
  mAutoSubTicks(true),
  mTickPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  mSelectedTickPen(QPen(Qt::blue, 2)),
  mSubTickPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  mSelectedSubTickPen(QPen(Qt::blue, 2)),

  mRange(0, 5),
  mRangeReversed(false),
  mScaleType(stLinear),
  mScaleLogBase(10),
  mScaleLogBaseLogInv(1.0/qLn(mScaleLogBase)),

  mGrid(new QCPGrid(this)),
  mAxisPainter(new QCPAxisPainterPrivate(parent->parentPlot())),
  mLowestVisibleTick(0),
  mHighestVisibleTick(-1),
  mCachedMarginValid(false),
  mCachedMargin(0)
{
  mGrid->setVisible(false);
  setAntialiased(false);
  setLayer(mParentPlot->currentLayer());

  if (type == atTop)
  {
    setTickLabelPadding(3);
    setLabelPadding(6);
  } else if (type == atRight)
  {
    setTickLabelPadding(7);
    setLabelPadding(12);
  } else if (type == atBottom)
  {
    setTickLabelPadding(3);
    setLabelPadding(3);
  } else if (type == atLeft)
  {
    setTickLabelPadding(5);
    setLabelPadding(10);
  }
}

QCPAxis::~QCPAxis()
{
  delete mAxisPainter;
}

int QCPAxis::tickLabelPadding() const
{
  return mAxisPainter->tickLabelPadding;
}

double QCPAxis::tickLabelRotation() const
{
  return mAxisPainter->tickLabelRotation;
}

QString QCPAxis::numberFormat() const
{
  QString result;
  result.append(mNumberFormatChar);
  if (mNumberBeautifulPowers)
  {
    result.append("b");
    if (mAxisPainter->numberMultiplyCross)
      result.append("c");
  }
  return result;
}

int QCPAxis::tickLengthIn() const
{
  return mAxisPainter->tickLengthIn;
}

int QCPAxis::tickLengthOut() const
{
  return mAxisPainter->tickLengthOut;
}

int QCPAxis::subTickLengthIn() const
{
  return mAxisPainter->subTickLengthIn;
}

int QCPAxis::subTickLengthOut() const
{
  return mAxisPainter->subTickLengthOut;
}

int QCPAxis::labelPadding() const
{
  return mAxisPainter->labelPadding;
}

int QCPAxis::offset() const
{
  return mAxisPainter->offset;
}

QCPLineEnding QCPAxis::lowerEnding() const
{
  return mAxisPainter->lowerEnding;
}

QCPLineEnding QCPAxis::upperEnding() const
{
  return mAxisPainter->upperEnding;
}

void QCPAxis::setScaleType(QCPAxis::ScaleType type)
{
  if (mScaleType != type)
  {
    mScaleType = type;
    if (mScaleType == stLogarithmic)
      setRange(mRange.sanitizedForLogScale());
    mCachedMarginValid = false;
    Q_EMIT scaleTypeChanged(mScaleType);
  }
}

void QCPAxis::setScaleLogBase(double base)
{
  if (base > 1)
  {
    mScaleLogBase = base;
    mScaleLogBaseLogInv = 1.0/qLn(mScaleLogBase);
    mCachedMarginValid = false;
  } else
    qDebug() << Q_FUNC_INFO << "Invalid logarithmic scale base (must be greater 1):" << base;
}

void QCPAxis::setRange(const QCPRange &range)
{
  if (range.lower == mRange.lower && range.upper == mRange.upper)
    return;

  if (!QCPRange::validRange(range)) return;
  QCPRange oldRange = mRange;
  if (mScaleType == stLogarithmic)
  {
    mRange = range.sanitizedForLogScale();
  } else
  {
    mRange = range.sanitizedForLinScale();
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::setSelectableParts(const SelectableParts &selectable)
{
  if (mSelectableParts != selectable)
  {
    mSelectableParts = selectable;
    Q_EMIT selectableChanged(mSelectableParts);
  }
}

void QCPAxis::setSelectedParts(const SelectableParts &selected)
{
  if (mSelectedParts != selected)
  {
    mSelectedParts = selected;
    Q_EMIT selectionChanged(mSelectedParts);
  }
}

void QCPAxis::setRange(double lower, double upper)
{
  if (lower == mRange.lower && upper == mRange.upper)
    return;

  if (!QCPRange::validRange(lower, upper)) return;
  QCPRange oldRange = mRange;
  mRange.lower = lower;
  mRange.upper = upper;
  if (mScaleType == stLogarithmic)
  {
    mRange = mRange.sanitizedForLogScale();
  } else
  {
    mRange = mRange.sanitizedForLinScale();
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::setRange(double position, double size, Qt::AlignmentFlag alignment)
{
  if (alignment == Qt::AlignLeft)
    setRange(position, position+size);
  else if (alignment == Qt::AlignRight)
    setRange(position-size, position);
  else
    setRange(position-size/2.0, position+size/2.0);
}

void QCPAxis::setRangeLower(double lower)
{
  if (mRange.lower == lower)
    return;

  QCPRange oldRange = mRange;
  mRange.lower = lower;
  if (mScaleType == stLogarithmic)
  {
    mRange = mRange.sanitizedForLogScale();
  } else
  {
    mRange = mRange.sanitizedForLinScale();
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::setRangeUpper(double upper)
{
  if (mRange.upper == upper)
    return;

  QCPRange oldRange = mRange;
  mRange.upper = upper;
  if (mScaleType == stLogarithmic)
  {
    mRange = mRange.sanitizedForLogScale();
  } else
  {
    mRange = mRange.sanitizedForLinScale();
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::setRangeReversed(bool reversed)
{
  if (mRangeReversed != reversed)
  {
    mRangeReversed = reversed;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setAutoTicks(bool on)
{
  if (mAutoTicks != on)
  {
    mAutoTicks = on;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setAutoTickCount(int approximateCount)
{
  if (mAutoTickCount != approximateCount)
  {
    if (approximateCount > 0)
    {
      mAutoTickCount = approximateCount;
      mCachedMarginValid = false;
    } else
      qDebug() << Q_FUNC_INFO << "approximateCount must be greater than zero:" << approximateCount;
  }
}

void QCPAxis::setAutoTickLabels(bool on)
{
  if (mAutoTickLabels != on)
  {
    mAutoTickLabels = on;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setAutoTickStep(bool on)
{
  if (mAutoTickStep != on)
  {
    mAutoTickStep = on;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setAutoSubTicks(bool on)
{
  if (mAutoSubTicks != on)
  {
    mAutoSubTicks = on;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTicks(bool show)
{
  if (mTicks != show)
  {
    mTicks = show;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabels(bool show)
{
  if (mTickLabels != show)
  {
    mTickLabels = show;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabelPadding(int padding)
{
  if (mAxisPainter->tickLabelPadding != padding)
  {
    mAxisPainter->tickLabelPadding = padding;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabelType(LabelType type)
{
  if (mTickLabelType != type)
  {
    mTickLabelType = type;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabelFont(const QFont &font)
{
  if (font != mTickLabelFont)
  {
    mTickLabelFont = font;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabelColor(const QColor &color)
{
  if (color != mTickLabelColor)
  {
    mTickLabelColor = color;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickLabelRotation(double degrees)
{
  if (!qFuzzyIsNull(degrees-mAxisPainter->tickLabelRotation))
  {
    mAxisPainter->tickLabelRotation = qBound(-90.0, degrees, 90.0);
    mCachedMarginValid = false;
  }
}

void QCPAxis::setDateTimeFormat(const QString &format)
{
  if (mDateTimeFormat != format)
  {
    mDateTimeFormat = format;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setDateTimeSpec(const Qt::TimeSpec &timeSpec)
{
  mDateTimeSpec = timeSpec;
}

void QCPAxis::setNumberFormat(const QString &formatCode)
{
  if (formatCode.isEmpty())
  {
    qDebug() << Q_FUNC_INFO << "Passed formatCode is empty";
    return;
  }
  mCachedMarginValid = false;

  QString allowedFormatChars = "eEfgG";
  if (allowedFormatChars.contains(formatCode.at(0)))
  {
    mNumberFormatChar = formatCode.at(0).toLatin1();
  } else
  {
    qDebug() << Q_FUNC_INFO << "Invalid number format code (first char not in 'eEfgG'):" << formatCode;
    return;
  }
  if (formatCode.length() < 2)
  {
    mNumberBeautifulPowers = false;
    mAxisPainter->numberMultiplyCross = false;
    return;
  }

  if (formatCode.at(1) == 'b' && (mNumberFormatChar == 'e' || mNumberFormatChar == 'g'))
  {
    mNumberBeautifulPowers = true;
  } else
  {
    qDebug() << Q_FUNC_INFO << "Invalid number format code (second char not 'b' or first char neither 'e' nor 'g'):" << formatCode;
    return;
  }
  if (formatCode.length() < 3)
  {
    mAxisPainter->numberMultiplyCross = false;
    return;
  }

  if (formatCode.at(2) == 'c')
  {
    mAxisPainter->numberMultiplyCross = true;
  } else if (formatCode.at(2) == 'd')
  {
    mAxisPainter->numberMultiplyCross = false;
  } else
  {
    qDebug() << Q_FUNC_INFO << "Invalid number format code (third char neither 'c' nor 'd'):" << formatCode;
    return;
  }
}

void QCPAxis::setNumberPrecision(int precision)
{
  if (mNumberPrecision != precision)
  {
    mNumberPrecision = precision;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickStep(double step)
{
  if (mTickStep != step)
  {
    mTickStep = step;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setTickVector(const QVector<double> &vec)
{

  mTickVector = vec;
  mCachedMarginValid = false;
}

void QCPAxis::setTickVectorLabels(const QVector<QString> &vec)
{

  mTickVectorLabels = vec;
  mCachedMarginValid = false;
}

void QCPAxis::setTickLength(int inside, int outside)
{
  setTickLengthIn(inside);
  setTickLengthOut(outside);
}

void QCPAxis::setTickLengthIn(int inside)
{
  if (mAxisPainter->tickLengthIn != inside)
  {
    mAxisPainter->tickLengthIn = inside;
  }
}

void QCPAxis::setTickLengthOut(int outside)
{
  if (mAxisPainter->tickLengthOut != outside)
  {
    mAxisPainter->tickLengthOut = outside;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setSubTickCount(int count)
{
  mSubTickCount = count;
}

void QCPAxis::setSubTickLength(int inside, int outside)
{
  setSubTickLengthIn(inside);
  setSubTickLengthOut(outside);
}

void QCPAxis::setSubTickLengthIn(int inside)
{
  if (mAxisPainter->subTickLengthIn != inside)
  {
    mAxisPainter->subTickLengthIn = inside;
  }
}

void QCPAxis::setSubTickLengthOut(int outside)
{
  if (mAxisPainter->subTickLengthOut != outside)
  {
    mAxisPainter->subTickLengthOut = outside;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setBasePen(const QPen &pen)
{
  mBasePen = pen;
}

void QCPAxis::setTickPen(const QPen &pen)
{
  mTickPen = pen;
}

void QCPAxis::setSubTickPen(const QPen &pen)
{
  mSubTickPen = pen;
}

void QCPAxis::setLabelFont(const QFont &font)
{
  if (mLabelFont != font)
  {
    mLabelFont = font;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setLabelColor(const QColor &color)
{
  mLabelColor = color;
}

void QCPAxis::setLabel(const QString &str)
{
  if (mLabel != str)
  {
    mLabel = str;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setLabelPadding(int padding)
{
  if (mAxisPainter->labelPadding != padding)
  {
    mAxisPainter->labelPadding = padding;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setPadding(int padding)
{
  if (mPadding != padding)
  {
    mPadding = padding;
    mCachedMarginValid = false;
  }
}

void QCPAxis::setOffset(int offset)
{
  mAxisPainter->offset = offset;
}

void QCPAxis::setSelectedTickLabelFont(const QFont &font)
{
  if (font != mSelectedTickLabelFont)
  {
    mSelectedTickLabelFont = font;

  }
}

void QCPAxis::setSelectedLabelFont(const QFont &font)
{
  mSelectedLabelFont = font;

}

void QCPAxis::setSelectedTickLabelColor(const QColor &color)
{
  if (color != mSelectedTickLabelColor)
  {
    mSelectedTickLabelColor = color;
  }
}

void QCPAxis::setSelectedLabelColor(const QColor &color)
{
  mSelectedLabelColor = color;
}

void QCPAxis::setSelectedBasePen(const QPen &pen)
{
  mSelectedBasePen = pen;
}

void QCPAxis::setSelectedTickPen(const QPen &pen)
{
  mSelectedTickPen = pen;
}

void QCPAxis::setSelectedSubTickPen(const QPen &pen)
{
  mSelectedSubTickPen = pen;
}

void QCPAxis::setLowerEnding(const QCPLineEnding &ending)
{
  mAxisPainter->lowerEnding = ending;
}

void QCPAxis::setUpperEnding(const QCPLineEnding &ending)
{
  mAxisPainter->upperEnding = ending;
}

void QCPAxis::moveRange(double diff)
{
  QCPRange oldRange = mRange;
  if (mScaleType == stLinear)
  {
    mRange.lower += diff;
    mRange.upper += diff;
  } else
  {
    mRange.lower *= diff;
    mRange.upper *= diff;
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::scaleRange(double factor, double center)
{
  QCPRange oldRange = mRange;
  if (mScaleType == stLinear)
  {
    QCPRange newRange;
    newRange.lower = (mRange.lower-center)*factor + center;
    newRange.upper = (mRange.upper-center)*factor + center;
    if (QCPRange::validRange(newRange))
      mRange = newRange.sanitizedForLinScale();
  } else
  {
    if ((mRange.upper < 0 && center < 0) || (mRange.upper > 0 && center > 0))
    {
      QCPRange newRange;
      newRange.lower = pow(mRange.lower/center, factor)*center;
      newRange.upper = pow(mRange.upper/center, factor)*center;
      if (QCPRange::validRange(newRange))
        mRange = newRange.sanitizedForLogScale();
    } else
      qDebug() << Q_FUNC_INFO << "Center of scaling operation doesn't lie in same logarithmic sign domain as range:" << center;
  }
  mCachedMarginValid = false;
  Q_EMIT rangeChanged(mRange);
  Q_EMIT rangeChanged(mRange, oldRange);
}

void QCPAxis::setScaleRatio(const QCPAxis *otherAxis, double ratio)
{
  int otherPixelSize, ownPixelSize;

  if (otherAxis->orientation() == Qt::Horizontal)
    otherPixelSize = otherAxis->axisRect()->width();
  else
    otherPixelSize = otherAxis->axisRect()->height();

  if (orientation() == Qt::Horizontal)
    ownPixelSize = axisRect()->width();
  else
    ownPixelSize = axisRect()->height();

  double newRangeSize = ratio*otherAxis->range().size()*ownPixelSize/(double)otherPixelSize;
  setRange(range().center(), newRangeSize, Qt::AlignCenter);
}

void QCPAxis::rescale(bool onlyVisiblePlottables)
{
  QList<QCPAbstractPlottable*> p = plottables();
  QCPRange newRange;
  bool haveRange = false;
  for (int i=0; i<p.size(); ++i)
  {
    if (!p.at(i)->realVisibility() && onlyVisiblePlottables)
      continue;
    QCPRange plottableRange;
    bool currentFoundRange;
    QCPAbstractPlottable::SignDomain signDomain = QCPAbstractPlottable::sdBoth;
    if (mScaleType == stLogarithmic)
      signDomain = (mRange.upper < 0 ? QCPAbstractPlottable::sdNegative : QCPAbstractPlottable::sdPositive);
    if (p.at(i)->keyAxis() == this)
      plottableRange = p.at(i)->getKeyRange(currentFoundRange, signDomain);
    else
      plottableRange = p.at(i)->getValueRange(currentFoundRange, signDomain);
    if (currentFoundRange)
    {
      if (!haveRange)
        newRange = plottableRange;
      else
        newRange.expand(plottableRange);
      haveRange = true;
    }
  }
  if (haveRange)
  {
    if (!QCPRange::validRange(newRange))
    {
      double center = (newRange.lower+newRange.upper)*0.5;
      if (mScaleType == stLinear)
      {
        newRange.lower = center-mRange.size()/2.0;
        newRange.upper = center+mRange.size()/2.0;
      } else
      {
        newRange.lower = center/qSqrt(mRange.upper/mRange.lower);
        newRange.upper = center*qSqrt(mRange.upper/mRange.lower);
      }
    }
    setRange(newRange);
  }
}

double QCPAxis::pixelToCoord(double value) const
{
  if (orientation() == Qt::Horizontal)
  {
    if (mScaleType == stLinear)
    {
      if (!mRangeReversed)
        return (value-mAxisRect->left())/(double)mAxisRect->width()*mRange.size()+mRange.lower;
      else
        return -(value-mAxisRect->left())/(double)mAxisRect->width()*mRange.size()+mRange.upper;
    } else
    {
      if (!mRangeReversed)
        return pow(mRange.upper/mRange.lower, (value-mAxisRect->left())/(double)mAxisRect->width())*mRange.lower;
      else
        return pow(mRange.upper/mRange.lower, (mAxisRect->left()-value)/(double)mAxisRect->width())*mRange.upper;
    }
  } else
  {
    if (mScaleType == stLinear)
    {
      if (!mRangeReversed)
        return (mAxisRect->bottom()-value)/(double)mAxisRect->height()*mRange.size()+mRange.lower;
      else
        return -(mAxisRect->bottom()-value)/(double)mAxisRect->height()*mRange.size()+mRange.upper;
    } else
    {
      if (!mRangeReversed)
        return pow(mRange.upper/mRange.lower, (mAxisRect->bottom()-value)/(double)mAxisRect->height())*mRange.lower;
      else
        return pow(mRange.upper/mRange.lower, (value-mAxisRect->bottom())/(double)mAxisRect->height())*mRange.upper;
    }
  }
}

double QCPAxis::coordToPixel(double value) const
{
  if (orientation() == Qt::Horizontal)
  {
    if (mScaleType == stLinear)
    {
      if (!mRangeReversed)
        return (value-mRange.lower)/mRange.size()*mAxisRect->width()+mAxisRect->left();
      else
        return (mRange.upper-value)/mRange.size()*mAxisRect->width()+mAxisRect->left();
    } else
    {
      if (value >= 0 && mRange.upper < 0)
        return !mRangeReversed ? mAxisRect->right()+200 : mAxisRect->left()-200;
      else if (value <= 0 && mRange.upper > 0)
        return !mRangeReversed ? mAxisRect->left()-200 : mAxisRect->right()+200;
      else
      {
        if (!mRangeReversed)
          return baseLog(value/mRange.lower)/baseLog(mRange.upper/mRange.lower)*mAxisRect->width()+mAxisRect->left();
        else
          return baseLog(mRange.upper/value)/baseLog(mRange.upper/mRange.lower)*mAxisRect->width()+mAxisRect->left();
      }
    }
  } else
  {
    if (mScaleType == stLinear)
    {
      if (!mRangeReversed)
        return mAxisRect->bottom()-(value-mRange.lower)/mRange.size()*mAxisRect->height();
      else
        return mAxisRect->bottom()-(mRange.upper-value)/mRange.size()*mAxisRect->height();
    } else
    {
      if (value >= 0 && mRange.upper < 0)
        return !mRangeReversed ? mAxisRect->top()-200 : mAxisRect->bottom()+200;
      else if (value <= 0 && mRange.upper > 0)
        return !mRangeReversed ? mAxisRect->bottom()+200 : mAxisRect->top()-200;
      else
      {
        if (!mRangeReversed)
          return mAxisRect->bottom()-baseLog(value/mRange.lower)/baseLog(mRange.upper/mRange.lower)*mAxisRect->height();
        else
          return mAxisRect->bottom()-baseLog(mRange.upper/value)/baseLog(mRange.upper/mRange.lower)*mAxisRect->height();
      }
    }
  }
}

QCPAxis::SelectablePart QCPAxis::getPartAt(const QPointF &pos) const
{
  if (!mVisible)
    return spNone;

  if (mAxisPainter->axisSelectionBox().contains(pos.toPoint()))
    return spAxis;
  else if (mAxisPainter->tickLabelsSelectionBox().contains(pos.toPoint()))
    return spTickLabels;
  else if (mAxisPainter->labelSelectionBox().contains(pos.toPoint()))
    return spAxisLabel;
  else
    return spNone;
}

double QCPAxis::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  if (!mParentPlot) return -1;
  SelectablePart part = getPartAt(pos);
  if ((onlySelectable && !mSelectableParts.testFlag(part)) || part == spNone)
    return -1;

  if (details)
    details->setValue(part);
  return mParentPlot->selectionTolerance()*0.99;
}

QList<QCPAbstractPlottable*> QCPAxis::plottables() const
{
  QList<QCPAbstractPlottable*> result;
  if (!mParentPlot) return result;

  for (int i=0; i<mParentPlot->mPlottables.size(); ++i)
  {
    if (mParentPlot->mPlottables.at(i)->keyAxis() == this ||mParentPlot->mPlottables.at(i)->valueAxis() == this)
      result.append(mParentPlot->mPlottables.at(i));
  }
  return result;
}

QList<QCPGraph*> QCPAxis::graphs() const
{
  QList<QCPGraph*> result;
  if (!mParentPlot) return result;

  for (int i=0; i<mParentPlot->mGraphs.size(); ++i)
  {
    if (mParentPlot->mGraphs.at(i)->keyAxis() == this || mParentPlot->mGraphs.at(i)->valueAxis() == this)
      result.append(mParentPlot->mGraphs.at(i));
  }
  return result;
}

QList<QCPAbstractItem*> QCPAxis::items() const
{
  QList<QCPAbstractItem*> result;
  if (!mParentPlot) return result;

  for (int itemId=0; itemId<mParentPlot->mItems.size(); ++itemId)
  {
    QList<QCPItemPosition*> positions = mParentPlot->mItems.at(itemId)->positions();
    for (int posId=0; posId<positions.size(); ++posId)
    {
      if (positions.at(posId)->keyAxis() == this || positions.at(posId)->valueAxis() == this)
      {
        result.append(mParentPlot->mItems.at(itemId));
        break;
      }
    }
  }
  return result;
}

QCPAxis::AxisType QCPAxis::marginSideToAxisType(QCP::MarginSide side)
{
  switch (side)
  {
    case QCP::msLeft: return atLeft;
    case QCP::msRight: return atRight;
    case QCP::msTop: return atTop;
    case QCP::msBottom: return atBottom;
    default: break;
  }
  qDebug() << Q_FUNC_INFO << "Invalid margin side passed:" << (int)side;
  return atLeft;
}

QCPAxis::AxisType QCPAxis::opposite(QCPAxis::AxisType type)
{
  switch (type)
  {
    case atLeft: return atRight; break;
    case atRight: return atLeft; break;
    case atBottom: return atTop; break;
    case atTop: return atBottom; break;
    default: qDebug() << Q_FUNC_INFO << "invalid axis type"; return atLeft; break;
  }
}

void QCPAxis::setupTickVectors()
{
  if (!mParentPlot) return;
  if ((!mTicks && !mTickLabels && !mGrid->visible()) || mRange.size() <= 0) return;

  if (mAutoTicks)
  {
    generateAutoTicks();
  } else
  {
    Q_EMIT ticksRequest();
  }

  visibleTickBounds(mLowestVisibleTick, mHighestVisibleTick);
  if (mTickVector.isEmpty())
  {
    mSubTickVector.clear();
    return;
  }

  mSubTickVector.resize((mTickVector.size()-1)*mSubTickCount);
  if (mSubTickCount > 0)
  {
    double subTickStep = 0;
    double subTickPosition = 0;
    int subTickIndex = 0;
    bool done = false;
    int lowTick = mLowestVisibleTick > 0 ? mLowestVisibleTick-1 : mLowestVisibleTick;
    int highTick = mHighestVisibleTick < mTickVector.size()-1 ? mHighestVisibleTick+1 : mHighestVisibleTick;
    for (int i=lowTick+1; i<=highTick; ++i)
    {
      subTickStep = (mTickVector.at(i)-mTickVector.at(i-1))/(double)(mSubTickCount+1);
      for (int k=1; k<=mSubTickCount; ++k)
      {
        subTickPosition = mTickVector.at(i-1) + k*subTickStep;
        if (subTickPosition < mRange.lower)
          continue;
        if (subTickPosition > mRange.upper)
        {
          done = true;
          break;
        }
        mSubTickVector[subTickIndex] = subTickPosition;
        subTickIndex++;
      }
      if (done) break;
    }
    mSubTickVector.resize(subTickIndex);
  }

  if (mAutoTickLabels)
  {
    int vecsize = mTickVector.size();
    mTickVectorLabels.resize(vecsize);
    if (mTickLabelType == ltNumber)
    {
      for (int i=mLowestVisibleTick; i<=mHighestVisibleTick; ++i)
        mTickVectorLabels[i] = mParentPlot->locale().toString(mTickVector.at(i), mNumberFormatChar, mNumberPrecision);
    } else if (mTickLabelType == ltDateTime)
    {
      for (int i=mLowestVisibleTick; i<=mHighestVisibleTick; ++i)
      {
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
        mTickVectorLabels[i] = mParentPlot->locale().toString(QDateTime::fromTime_t(mTickVector.at(i)).toTimeSpec(mDateTimeSpec), mDateTimeFormat);
#else
        mTickVectorLabels[i] = mParentPlot->locale().toString(QDateTime::fromMSecsSinceEpoch(mTickVector.at(i)*1000).toTimeSpec(mDateTimeSpec), mDateTimeFormat);
#endif
      }
    }
  } else
  {
    if (mAutoTicks)
    {
      Q_EMIT ticksRequest();
    }

    if (mTickVectorLabels.size() < mTickVector.size())
      mTickVectorLabels.resize(mTickVector.size());
  }
}

void QCPAxis::generateAutoTicks()
{
  if (mScaleType == stLinear)
  {
    if (mAutoTickStep)
    {

      mTickStep = mRange.size()/(double)(mAutoTickCount+1e-10);
      double magnitudeFactor = qPow(10.0, qFloor(qLn(mTickStep)/qLn(10.0)));
      double tickStepMantissa = mTickStep/magnitudeFactor;
      if (tickStepMantissa < 5)
      {

        mTickStep = (int)(tickStepMantissa*2)/2.0*magnitudeFactor;
      } else
      {

        mTickStep = (int)(tickStepMantissa/2.0)*2.0*magnitudeFactor;
      }
    }
    if (mAutoSubTicks)
      mSubTickCount = calculateAutoSubTickCount(mTickStep);

    qint64 firstStep = floor(mRange.lower/mTickStep);
    qint64 lastStep = ceil(mRange.upper/mTickStep);
    int tickcount = lastStep-firstStep+1;
    if (tickcount < 0) tickcount = 0;
    mTickVector.resize(tickcount);
    for (int i=0; i<tickcount; ++i)
      mTickVector[i] = (firstStep+i)*mTickStep;
  } else
  {

    if (mRange.lower > 0 && mRange.upper > 0)
    {
      double lowerMag = basePow((int)floor(baseLog(mRange.lower)));
      double currentMag = lowerMag;
      mTickVector.clear();
      mTickVector.append(currentMag);
      while (currentMag < mRange.upper && currentMag > 0)
      {
        currentMag *= mScaleLogBase;
        mTickVector.append(currentMag);
      }
    } else if (mRange.lower < 0 && mRange.upper < 0)
    {
      double lowerMag = -basePow((int)ceil(baseLog(-mRange.lower)));
      double currentMag = lowerMag;
      mTickVector.clear();
      mTickVector.append(currentMag);
      while (currentMag < mRange.upper && currentMag < 0)
      {
        currentMag /= mScaleLogBase;
        mTickVector.append(currentMag);
      }
    } else
    {
      mTickVector.clear();
      qDebug() << Q_FUNC_INFO << "Invalid range for logarithmic plot: " << mRange.lower << "-" << mRange.upper;
    }
  }
}

int QCPAxis::calculateAutoSubTickCount(double tickStep) const
{
  int result = mSubTickCount;

  double magnitudeFactor = qPow(10.0, qFloor(qLn(tickStep)/qLn(10.0)));
  double tickStepMantissa = tickStep/magnitudeFactor;

  double epsilon = 0.01;
  double intPartf;
  int intPart;
  double fracPart = modf(tickStepMantissa, &intPartf);
  intPart = intPartf;

  if (fracPart < epsilon || 1.0-fracPart < epsilon)
  {
    if (1.0-fracPart < epsilon)
      ++intPart;
    switch (intPart)
    {
      case 1: result = 4; break;
      case 2: result = 3; break;
      case 3: result = 2; break;
      case 4: result = 3; break;
      case 5: result = 4; break;
      case 6: result = 2; break;
      case 7: result = 6; break;
      case 8: result = 3; break;
      case 9: result = 2; break;
    }
  } else
  {

    if (qAbs(fracPart-0.5) < epsilon)
    {
      switch (intPart)
      {
        case 1: result = 2; break;
        case 2: result = 4; break;
        case 3: result = 4; break;
        case 4: result = 2; break;
        case 5: result = 4; break;
        case 6: result = 4; break;
        case 7: result = 2; break;
        case 8: result = 4; break;
        case 9: result = 4; break;
      }
    }

  }

  return result;
}

void QCPAxis::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  SelectablePart part = details.value<SelectablePart>();
  if (mSelectableParts.testFlag(part))
  {
    SelectableParts selBefore = mSelectedParts;
    setSelectedParts(additive ? mSelectedParts^part : part);
    if (selectionStateChanged)
      *selectionStateChanged = mSelectedParts != selBefore;
  }
}

void QCPAxis::deselectEvent(bool *selectionStateChanged)
{
  SelectableParts selBefore = mSelectedParts;
  setSelectedParts(mSelectedParts & ~mSelectableParts);
  if (selectionStateChanged)
    *selectionStateChanged = mSelectedParts != selBefore;
}

void QCPAxis::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeAxes);
}

void QCPAxis::draw(QCPPainter *painter)
{
  const int lowTick = mLowestVisibleTick;
  const int highTick = mHighestVisibleTick;
  QVector<double> subTickPositions;
  QVector<double> tickPositions;
  QVector<QString> tickLabels;
  tickPositions.reserve(highTick-lowTick+1);
  tickLabels.reserve(highTick-lowTick+1);
  subTickPositions.reserve(mSubTickVector.size());

  if (mTicks)
  {
    for (int i=lowTick; i<=highTick; ++i)
    {
      tickPositions.append(coordToPixel(mTickVector.at(i)));
      if (mTickLabels)
        tickLabels.append(mTickVectorLabels.at(i));
    }

    if (mSubTickCount > 0)
    {
      const int subTickCount = mSubTickVector.size();
      for (int i=0; i<subTickCount; ++i)
        subTickPositions.append(coordToPixel(mSubTickVector.at(i)));
    }
  }

  mAxisPainter->type = mAxisType;
  mAxisPainter->basePen = getBasePen();
  mAxisPainter->labelFont = getLabelFont();
  mAxisPainter->labelColor = getLabelColor();
  mAxisPainter->label = mLabel;
  mAxisPainter->substituteExponent = mAutoTickLabels && mNumberBeautifulPowers;
  mAxisPainter->tickPen = getTickPen();
  mAxisPainter->subTickPen = getSubTickPen();
  mAxisPainter->tickLabelFont = getTickLabelFont();
  mAxisPainter->tickLabelColor = getTickLabelColor();
  mAxisPainter->alignmentRect = mAxisRect->rect();
  mAxisPainter->viewportRect = mParentPlot->viewport();
  mAxisPainter->abbreviateDecimalPowers = mScaleType == stLogarithmic;
  mAxisPainter->reversedEndings = mRangeReversed;
  mAxisPainter->tickPositions = tickPositions;
  mAxisPainter->tickLabels = tickLabels;
  mAxisPainter->subTickPositions = subTickPositions;
  mAxisPainter->draw(painter);
}

void QCPAxis::visibleTickBounds(int &lowIndex, int &highIndex) const
{
  bool lowFound = false;
  bool highFound = false;
  lowIndex = 0;
  highIndex = -1;

  for (int i=0; i < mTickVector.size(); ++i)
  {
    if (mTickVector.at(i) >= mRange.lower)
    {
      lowFound = true;
      lowIndex = i;
      break;
    }
  }
  for (int i=mTickVector.size()-1; i >= 0; --i)
  {
    if (mTickVector.at(i) <= mRange.upper)
    {
      highFound = true;
      highIndex = i;
      break;
    }
  }

  if (!lowFound && highFound)
    lowIndex = highIndex+1;
  else if (lowFound && !highFound)
    highIndex = lowIndex-1;
}

double QCPAxis::baseLog(double value) const
{
  return qLn(value)*mScaleLogBaseLogInv;
}

double QCPAxis::basePow(double value) const
{
  return qPow(mScaleLogBase, value);
}

QPen QCPAxis::getBasePen() const
{
  return mSelectedParts.testFlag(spAxis) ? mSelectedBasePen : mBasePen;
}

QPen QCPAxis::getTickPen() const
{
  return mSelectedParts.testFlag(spAxis) ? mSelectedTickPen : mTickPen;
}

QPen QCPAxis::getSubTickPen() const
{
  return mSelectedParts.testFlag(spAxis) ? mSelectedSubTickPen : mSubTickPen;
}

QFont QCPAxis::getTickLabelFont() const
{
  return mSelectedParts.testFlag(spTickLabels) ? mSelectedTickLabelFont : mTickLabelFont;
}

QFont QCPAxis::getLabelFont() const
{
  return mSelectedParts.testFlag(spAxisLabel) ? mSelectedLabelFont : mLabelFont;
}

QColor QCPAxis::getTickLabelColor() const
{
  return mSelectedParts.testFlag(spTickLabels) ? mSelectedTickLabelColor : mTickLabelColor;
}

QColor QCPAxis::getLabelColor() const
{
  return mSelectedParts.testFlag(spAxisLabel) ? mSelectedLabelColor : mLabelColor;
}

int QCPAxis::calculateMargin()
{
  if (!mVisible)
    return 0;

  if (mCachedMarginValid)
    return mCachedMargin;

  int margin = 0;

  int lowTick, highTick;
  visibleTickBounds(lowTick, highTick);
  QVector<double> tickPositions;
  QVector<QString> tickLabels;
  tickPositions.reserve(highTick-lowTick+1);
  tickLabels.reserve(highTick-lowTick+1);
  if (mTicks)
  {
    for (int i=lowTick; i<=highTick; ++i)
    {
      tickPositions.append(coordToPixel(mTickVector.at(i)));
      if (mTickLabels)
        tickLabels.append(mTickVectorLabels.at(i));
    }
  }

  mAxisPainter->type = mAxisType;
  mAxisPainter->labelFont = getLabelFont();
  mAxisPainter->label = mLabel;
  mAxisPainter->tickLabelFont = mTickLabelFont;
  mAxisPainter->alignmentRect = mAxisRect->rect();
  mAxisPainter->viewportRect = mParentPlot->viewport();
  mAxisPainter->tickPositions = tickPositions;
  mAxisPainter->tickLabels = tickLabels;
  margin += mAxisPainter->size();
  margin += mPadding;

  mCachedMargin = margin;
  mCachedMarginValid = true;
  return margin;
}

QCP::Interaction QCPAxis::selectionCategory() const
{
  return QCP::iSelectAxes;
}

QCPAxisPainterPrivate::QCPAxisPainterPrivate(QCustomPlot *parentPlot) :
  type(QCPAxis::atLeft),
  basePen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  lowerEnding(QCPLineEnding::esNone),
  upperEnding(QCPLineEnding::esNone),
  labelPadding(0),
  tickLabelPadding(0),
  tickLabelRotation(0),
  substituteExponent(true),
  numberMultiplyCross(false),
  tickLengthIn(5),
  tickLengthOut(0),
  subTickLengthIn(2),
  subTickLengthOut(0),
  tickPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  subTickPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap)),
  offset(0),
  abbreviateDecimalPowers(false),
  reversedEndings(false),
  mParentPlot(parentPlot),
  mLabelCache(16)
{
}

QCPAxisPainterPrivate::~QCPAxisPainterPrivate()
{
}

void QCPAxisPainterPrivate::draw(QCPPainter *painter)
{
  QByteArray newHash = generateLabelParameterHash();
  if (newHash != mLabelParameterHash)
  {
    mLabelCache.clear();
    mLabelParameterHash = newHash;
  }

  QPoint origin;
  switch (type)
  {
    case QCPAxis::atLeft:   origin = alignmentRect.bottomLeft() +QPoint(-offset, 0); break;
    case QCPAxis::atRight:  origin = alignmentRect.bottomRight()+QPoint(+offset, 0); break;
    case QCPAxis::atTop:    origin = alignmentRect.topLeft()    +QPoint(0, -offset); break;
    case QCPAxis::atBottom: origin = alignmentRect.bottomLeft() +QPoint(0, +offset); break;
  }

  double xCor = 0, yCor = 0;
  switch (type)
  {
    case QCPAxis::atTop: yCor = -1; break;
    case QCPAxis::atRight: xCor = 1; break;
    default: break;
  }

  int margin = 0;

  QLineF baseLine;
  painter->setPen(basePen);
  if (QCPAxis::orientation(type) == Qt::Horizontal)
    baseLine.setPoints(origin+QPointF(xCor, yCor), origin+QPointF(alignmentRect.width()+xCor, yCor));
  else
    baseLine.setPoints(origin+QPointF(xCor, yCor), origin+QPointF(xCor, -alignmentRect.height()+yCor));
  if (reversedEndings)
    baseLine = QLineF(baseLine.p2(), baseLine.p1());
  painter->drawLine(baseLine);

  if (!tickPositions.isEmpty())
  {
    painter->setPen(tickPen);
    int tickDir = (type == QCPAxis::atBottom || type == QCPAxis::atRight) ? -1 : 1;
    if (QCPAxis::orientation(type) == Qt::Horizontal)
    {
      for (int i=0; i<tickPositions.size(); ++i)
        painter->drawLine(QLineF(tickPositions.at(i)+xCor, origin.y()-tickLengthOut*tickDir+yCor, tickPositions.at(i)+xCor, origin.y()+tickLengthIn*tickDir+yCor));
    } else
    {
      for (int i=0; i<tickPositions.size(); ++i)
        painter->drawLine(QLineF(origin.x()-tickLengthOut*tickDir+xCor, tickPositions.at(i)+yCor, origin.x()+tickLengthIn*tickDir+xCor, tickPositions.at(i)+yCor));
    }
  }

  if (!subTickPositions.isEmpty())
  {
    painter->setPen(subTickPen);

    int tickDir = (type == QCPAxis::atBottom || type == QCPAxis::atRight) ? -1 : 1;
    if (QCPAxis::orientation(type) == Qt::Horizontal)
    {
      for (int i=0; i<subTickPositions.size(); ++i)
        painter->drawLine(QLineF(subTickPositions.at(i)+xCor, origin.y()-subTickLengthOut*tickDir+yCor, subTickPositions.at(i)+xCor, origin.y()+subTickLengthIn*tickDir+yCor));
    } else
    {
      for (int i=0; i<subTickPositions.size(); ++i)
        painter->drawLine(QLineF(origin.x()-subTickLengthOut*tickDir+xCor, subTickPositions.at(i)+yCor, origin.x()+subTickLengthIn*tickDir+xCor, subTickPositions.at(i)+yCor));
    }
  }
  margin += qMax(0, qMax(tickLengthOut, subTickLengthOut));

  bool antialiasingBackup = painter->antialiasing();
  painter->setAntialiasing(true);
  painter->setBrush(QBrush(basePen.color()));
  QVector2D baseLineVector(baseLine.dx(), baseLine.dy());
  if (lowerEnding.style() != QCPLineEnding::esNone)
    lowerEnding.draw(painter, QVector2D(baseLine.p1())-baseLineVector.normalized()*lowerEnding.realLength()*(lowerEnding.inverted()?-1:1), -baseLineVector);
  if (upperEnding.style() != QCPLineEnding::esNone)
    upperEnding.draw(painter, QVector2D(baseLine.p2())+baseLineVector.normalized()*upperEnding.realLength()*(upperEnding.inverted()?-1:1), baseLineVector);
  painter->setAntialiasing(antialiasingBackup);

  QSize tickLabelsSize(0, 0);
  if (!tickLabels.isEmpty())
  {
    margin += tickLabelPadding;
    painter->setFont(tickLabelFont);
    painter->setPen(QPen(tickLabelColor));
    const int maxLabelIndex = qMin(tickPositions.size(), tickLabels.size());
    for (int i=0; i<maxLabelIndex; ++i)
      placeTickLabel(painter, tickPositions.at(i), margin, tickLabels.at(i), &tickLabelsSize);
    if (QCPAxis::orientation(type) == Qt::Horizontal)
      margin += tickLabelsSize.height();
    else
      margin += tickLabelsSize.width();
  }

  QRect labelBounds;
  if (!label.isEmpty())
  {
    margin += labelPadding;
    painter->setFont(labelFont);
    painter->setPen(QPen(labelColor));
    labelBounds = painter->fontMetrics().boundingRect(0, 0, 0, 0, Qt::TextDontClip, label);
    if (type == QCPAxis::atLeft)
    {
      QTransform oldTransform = painter->transform();
      painter->translate((origin.x()-margin-labelBounds.height()), origin.y());
      painter->rotate(-90);
      painter->drawText(0, 0, alignmentRect.height(), labelBounds.height(), Qt::TextDontClip | Qt::AlignCenter, label);
      painter->setTransform(oldTransform);
    }
    else if (type == QCPAxis::atRight)
    {
      QTransform oldTransform = painter->transform();
      painter->translate((origin.x()+margin+labelBounds.height()), origin.y()-alignmentRect.height());
      painter->rotate(90);
      painter->drawText(0, 0, alignmentRect.height(), labelBounds.height(), Qt::TextDontClip | Qt::AlignCenter, label);
      painter->setTransform(oldTransform);
    }
    else if (type == QCPAxis::atTop)
      painter->drawText(origin.x(), origin.y()-margin-labelBounds.height(), alignmentRect.width(), labelBounds.height(), Qt::TextDontClip | Qt::AlignCenter, label);
    else if (type == QCPAxis::atBottom)
      painter->drawText(origin.x(), origin.y()+margin, alignmentRect.width(), labelBounds.height(), Qt::TextDontClip | Qt::AlignCenter, label);
  }

  int selectionTolerance = 0;
  if (mParentPlot)
    selectionTolerance = mParentPlot->selectionTolerance();
  else
    qDebug() << Q_FUNC_INFO << "mParentPlot is null";
  int selAxisOutSize = qMax(qMax(tickLengthOut, subTickLengthOut), selectionTolerance);
  int selAxisInSize = selectionTolerance;
  int selTickLabelSize = (QCPAxis::orientation(type) == Qt::Horizontal ? tickLabelsSize.height() : tickLabelsSize.width());
  int selTickLabelOffset = qMax(tickLengthOut, subTickLengthOut)+tickLabelPadding;
  int selLabelSize = labelBounds.height();
  int selLabelOffset = selTickLabelOffset+selTickLabelSize+labelPadding;
  if (type == QCPAxis::atLeft)
  {
    mAxisSelectionBox.setCoords(origin.x()-selAxisOutSize, alignmentRect.top(), origin.x()+selAxisInSize, alignmentRect.bottom());
    mTickLabelsSelectionBox.setCoords(origin.x()-selTickLabelOffset-selTickLabelSize, alignmentRect.top(), origin.x()-selTickLabelOffset, alignmentRect.bottom());
    mLabelSelectionBox.setCoords(origin.x()-selLabelOffset-selLabelSize, alignmentRect.top(), origin.x()-selLabelOffset, alignmentRect.bottom());
  } else if (type == QCPAxis::atRight)
  {
    mAxisSelectionBox.setCoords(origin.x()-selAxisInSize, alignmentRect.top(), origin.x()+selAxisOutSize, alignmentRect.bottom());
    mTickLabelsSelectionBox.setCoords(origin.x()+selTickLabelOffset+selTickLabelSize, alignmentRect.top(), origin.x()+selTickLabelOffset, alignmentRect.bottom());
    mLabelSelectionBox.setCoords(origin.x()+selLabelOffset+selLabelSize, alignmentRect.top(), origin.x()+selLabelOffset, alignmentRect.bottom());
  } else if (type == QCPAxis::atTop)
  {
    mAxisSelectionBox.setCoords(alignmentRect.left(), origin.y()-selAxisOutSize, alignmentRect.right(), origin.y()+selAxisInSize);
    mTickLabelsSelectionBox.setCoords(alignmentRect.left(), origin.y()-selTickLabelOffset-selTickLabelSize, alignmentRect.right(), origin.y()-selTickLabelOffset);
    mLabelSelectionBox.setCoords(alignmentRect.left(), origin.y()-selLabelOffset-selLabelSize, alignmentRect.right(), origin.y()-selLabelOffset);
  } else if (type == QCPAxis::atBottom)
  {
    mAxisSelectionBox.setCoords(alignmentRect.left(), origin.y()-selAxisInSize, alignmentRect.right(), origin.y()+selAxisOutSize);
    mTickLabelsSelectionBox.setCoords(alignmentRect.left(), origin.y()+selTickLabelOffset+selTickLabelSize, alignmentRect.right(), origin.y()+selTickLabelOffset);
    mLabelSelectionBox.setCoords(alignmentRect.left(), origin.y()+selLabelOffset+selLabelSize, alignmentRect.right(), origin.y()+selLabelOffset);
  }

}

int QCPAxisPainterPrivate::size() const
{
  int result = 0;

  if (!tickPositions.isEmpty())
    result += qMax(0, qMax(tickLengthOut, subTickLengthOut));

  QSize tickLabelsSize(0, 0);
  if (!tickLabels.isEmpty())
  {
    for (int i=0; i<tickLabels.size(); ++i)
      getMaxTickLabelSize(tickLabelFont, tickLabels.at(i), &tickLabelsSize);
    result += QCPAxis::orientation(type) == Qt::Horizontal ? tickLabelsSize.height() : tickLabelsSize.width();
    result += tickLabelPadding;
  }

  if (!label.isEmpty())
  {
    QFontMetrics fontMetrics(labelFont);
    QRect bounds;
    bounds = fontMetrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip | Qt::AlignHCenter | Qt::AlignVCenter, label);
    result += bounds.height() + labelPadding;
  }

  return result;
}

void QCPAxisPainterPrivate::clearCache()
{
  mLabelCache.clear();
}

QByteArray QCPAxisPainterPrivate::generateLabelParameterHash() const
{
  QByteArray result;
  result.append(QByteArray::number(tickLabelRotation));
  result.append(QByteArray::number((int)substituteExponent));
  result.append(QByteArray::number((int)numberMultiplyCross));
  result.append(tickLabelColor.name()+QByteArray::number(tickLabelColor.alpha(), 16));
  result.append(tickLabelFont.toString());
  return result;
}

void QCPAxisPainterPrivate::placeTickLabel(QCPPainter *painter, double position, int distanceToAxis, const QString &text, QSize *tickLabelsSize)
{

  if (text.isEmpty()) return;
  QSize finalSize;
  QPointF labelAnchor;
  switch (type)
  {
    case QCPAxis::atLeft:   labelAnchor = QPointF(alignmentRect.left()-distanceToAxis-offset, position); break;
    case QCPAxis::atRight:  labelAnchor = QPointF(alignmentRect.right()+distanceToAxis+offset, position); break;
    case QCPAxis::atTop:    labelAnchor = QPointF(position, alignmentRect.top()-distanceToAxis-offset); break;
    case QCPAxis::atBottom: labelAnchor = QPointF(position, alignmentRect.bottom()+distanceToAxis+offset); break;
  }
  if (mParentPlot->plottingHints().testFlag(QCP::phCacheLabels) && !painter->modes().testFlag(QCPPainter::pmNoCaching))
  {
    if (!mLabelCache.contains(text))
    {
      CachedLabel *newCachedLabel = new CachedLabel;
      TickLabelData labelData = getTickLabelData(painter->font(), text);
      QPointF drawOffset = getTickLabelDrawOffset(labelData);
      newCachedLabel->offset = drawOffset+labelData.rotatedTotalBounds.topLeft();
      newCachedLabel->pixmap = QPixmap(labelData.rotatedTotalBounds.size());
      newCachedLabel->pixmap.fill(Qt::transparent);
      QCPPainter cachePainter(&newCachedLabel->pixmap);
      cachePainter.setPen(painter->pen());
      drawTickLabel(&cachePainter, -labelData.rotatedTotalBounds.topLeft().x(), -labelData.rotatedTotalBounds.topLeft().y(), labelData);
      mLabelCache.insert(text, newCachedLabel, 1);
    }

    const CachedLabel *cachedLabel = mLabelCache.object(text);

    if (QCPAxis::orientation(type) == Qt::Horizontal)
    {
      if (labelAnchor.x()+cachedLabel->offset.x()+cachedLabel->pixmap.width() > viewportRect.right() ||
          labelAnchor.x()+cachedLabel->offset.x() < viewportRect.left())
        return;
    } else
    {
      if (labelAnchor.y()+cachedLabel->offset.y()+cachedLabel->pixmap.height() >viewportRect.bottom() ||
          labelAnchor.y()+cachedLabel->offset.y() < viewportRect.top())
        return;
    }
    painter->drawPixmap(labelAnchor+cachedLabel->offset, cachedLabel->pixmap);
    finalSize = cachedLabel->pixmap.size();
  } else
  {
    TickLabelData labelData = getTickLabelData(painter->font(), text);
    QPointF finalPosition = labelAnchor + getTickLabelDrawOffset(labelData);

    if (QCPAxis::orientation(type) == Qt::Horizontal)
    {
      if (finalPosition.x()+(labelData.rotatedTotalBounds.width()+labelData.rotatedTotalBounds.left()) > viewportRect.right() ||
          finalPosition.x()+labelData.rotatedTotalBounds.left() < viewportRect.left())
        return;
    } else
    {
      if (finalPosition.y()+(labelData.rotatedTotalBounds.height()+labelData.rotatedTotalBounds.top()) > viewportRect.bottom() ||
          finalPosition.y()+labelData.rotatedTotalBounds.top() < viewportRect.top())
        return;
    }
    drawTickLabel(painter, finalPosition.x(), finalPosition.y(), labelData);
    finalSize = labelData.rotatedTotalBounds.size();
  }

  if (finalSize.width() > tickLabelsSize->width())
    tickLabelsSize->setWidth(finalSize.width());
  if (finalSize.height() > tickLabelsSize->height())
    tickLabelsSize->setHeight(finalSize.height());
}

void QCPAxisPainterPrivate::drawTickLabel(QCPPainter *painter, double x, double y, const TickLabelData &labelData) const
{

  QTransform oldTransform = painter->transform();
  QFont oldFont = painter->font();

  painter->translate(x, y);
  if (!qFuzzyIsNull(tickLabelRotation))
    painter->rotate(tickLabelRotation);

  if (!labelData.expPart.isEmpty())
  {
    painter->setFont(labelData.baseFont);
    painter->drawText(0, 0, 0, 0, Qt::TextDontClip, labelData.basePart);
    painter->setFont(labelData.expFont);
    painter->drawText(labelData.baseBounds.width()+1, 0, labelData.expBounds.width(), labelData.expBounds.height(), Qt::TextDontClip,  labelData.expPart);
  } else
  {
    painter->setFont(labelData.baseFont);
    painter->drawText(0, 0, labelData.totalBounds.width(), labelData.totalBounds.height(), Qt::TextDontClip | Qt::AlignHCenter, labelData.basePart);
  }

  painter->setTransform(oldTransform);
  painter->setFont(oldFont);
}

QCPAxisPainterPrivate::TickLabelData QCPAxisPainterPrivate::getTickLabelData(const QFont &font, const QString &text) const
{
  TickLabelData result;

  bool useBeautifulPowers = false;
  int ePos = -1;
  if (substituteExponent)
  {
    ePos = text.indexOf('e');
    if (ePos > -1)
      useBeautifulPowers = true;
  }

  result.baseFont = font;
  if (result.baseFont.pointSizeF() > 0)
    result.baseFont.setPointSizeF(result.baseFont.pointSizeF()+0.05);
  if (useBeautifulPowers)
  {

    result.basePart = text.left(ePos);

    if (abbreviateDecimalPowers && result.basePart == "1")
      result.basePart = "10";
    else
      result.basePart += (numberMultiplyCross ? QString(QChar(215)) : QString(QChar(183))) + "10";
    result.expPart = text.mid(ePos+1);

    while (result.expPart.length() > 2 && result.expPart.at(1) == '0')
      result.expPart.remove(1, 1);
    if (!result.expPart.isEmpty() && result.expPart.at(0) == '+')
      result.expPart.remove(0, 1);

    result.expFont = font;
    result.expFont.setPointSize(result.expFont.pointSize()*0.75);

    result.baseBounds = QFontMetrics(result.baseFont).boundingRect(0, 0, 0, 0, Qt::TextDontClip, result.basePart);
    result.expBounds = QFontMetrics(result.expFont).boundingRect(0, 0, 0, 0, Qt::TextDontClip, result.expPart);
    result.totalBounds = result.baseBounds.adjusted(0, 0, result.expBounds.width()+2, 0);
  } else
  {
    result.basePart = text;
    result.totalBounds = QFontMetrics(result.baseFont).boundingRect(0, 0, 0, 0, Qt::TextDontClip | Qt::AlignHCenter, result.basePart);
  }
  result.totalBounds.moveTopLeft(QPoint(0, 0));

  result.rotatedTotalBounds = result.totalBounds;
  if (!qFuzzyIsNull(tickLabelRotation))
  {
    QTransform transform;
    transform.rotate(tickLabelRotation);
    result.rotatedTotalBounds = transform.mapRect(result.rotatedTotalBounds);
  }

  return result;
}

QPointF QCPAxisPainterPrivate::getTickLabelDrawOffset(const TickLabelData &labelData) const
{

  bool doRotation = !qFuzzyIsNull(tickLabelRotation);
  bool flip = qFuzzyCompare(qAbs(tickLabelRotation), 90.0);
  double radians = tickLabelRotation/180.0*M_PI;
  int x=0, y=0;
  if (type == QCPAxis::atLeft)
  {
    if (doRotation)
    {
      if (tickLabelRotation > 0)
      {
        x = -qCos(radians)*labelData.totalBounds.width();
        y = flip ? -labelData.totalBounds.width()/2.0 : -qSin(radians)*labelData.totalBounds.width()-qCos(radians)*labelData.totalBounds.height()/2.0;
      } else
      {
        x = -qCos(-radians)*labelData.totalBounds.width()-qSin(-radians)*labelData.totalBounds.height();
        y = flip ? +labelData.totalBounds.width()/2.0 : +qSin(-radians)*labelData.totalBounds.width()-qCos(-radians)*labelData.totalBounds.height()/2.0;
      }
    } else
    {
      x = -labelData.totalBounds.width();
      y = -labelData.totalBounds.height()/2.0;
    }
  } else if (type == QCPAxis::atRight)
  {
    if (doRotation)
    {
      if (tickLabelRotation > 0)
      {
        x = +qSin(radians)*labelData.totalBounds.height();
        y = flip ? -labelData.totalBounds.width()/2.0 : -qCos(radians)*labelData.totalBounds.height()/2.0;
      } else
      {
        x = 0;
        y = flip ? +labelData.totalBounds.width()/2.0 : -qCos(-radians)*labelData.totalBounds.height()/2.0;
      }
    } else
    {
      x = 0;
      y = -labelData.totalBounds.height()/2.0;
    }
  } else if (type == QCPAxis::atTop)
  {
    if (doRotation)
    {
      if (tickLabelRotation > 0)
      {
        x = -qCos(radians)*labelData.totalBounds.width()+qSin(radians)*labelData.totalBounds.height()/2.0;
        y = -qSin(radians)*labelData.totalBounds.width()-qCos(radians)*labelData.totalBounds.height();
      } else
      {
        x = -qSin(-radians)*labelData.totalBounds.height()/2.0;
        y = -qCos(-radians)*labelData.totalBounds.height();
      }
    } else
    {
      x = -labelData.totalBounds.width()/2.0;
      y = -labelData.totalBounds.height();
    }
  } else if (type == QCPAxis::atBottom)
  {
    if (doRotation)
    {
      if (tickLabelRotation > 0)
      {
        x = +qSin(radians)*labelData.totalBounds.height()/2.0;
        y = 0;
      } else
      {
        x = -qCos(-radians)*labelData.totalBounds.width()-qSin(-radians)*labelData.totalBounds.height()/2.0;
        y = +qSin(-radians)*labelData.totalBounds.width();
      }
    } else
    {
      x = -labelData.totalBounds.width()/2.0;
      y = 0;
    }
  }

  return QPointF(x, y);
}

void QCPAxisPainterPrivate::getMaxTickLabelSize(const QFont &font, const QString &text,  QSize *tickLabelsSize) const
{

  QSize finalSize;
  if (mParentPlot->plottingHints().testFlag(QCP::phCacheLabels) && mLabelCache.contains(text))
  {
    const CachedLabel *cachedLabel = mLabelCache.object(text);
    finalSize = cachedLabel->pixmap.size();
  } else
  {
    TickLabelData labelData = getTickLabelData(font, text);
    finalSize = labelData.rotatedTotalBounds.size();
  }

  if (finalSize.width() > tickLabelsSize->width())
    tickLabelsSize->setWidth(finalSize.width());
  if (finalSize.height() > tickLabelsSize->height())
    tickLabelsSize->setHeight(finalSize.height());
}

QCPAbstractPlottable::QCPAbstractPlottable(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPLayerable(keyAxis->parentPlot(), "", keyAxis->axisRect()),
  mName(""),
  mAntialiasedFill(true),
  mAntialiasedScatters(true),
  mAntialiasedErrorBars(false),
  mPen(Qt::black),
  mSelectedPen(Qt::black),
  mBrush(Qt::NoBrush),
  mSelectedBrush(Qt::NoBrush),
  mKeyAxis(keyAxis),
  mValueAxis(valueAxis),
  mSelectable(true),
  mSelected(false)
{
  if (keyAxis->parentPlot() != valueAxis->parentPlot())
    qDebug() << Q_FUNC_INFO << "Parent plot of keyAxis is not the same as that of valueAxis.";
  if (keyAxis->orientation() == valueAxis->orientation())
    qDebug() << Q_FUNC_INFO << "keyAxis and valueAxis must be orthogonal to each other.";
}

void QCPAbstractPlottable::setName(const QString &name)
{
  mName = name;
}

void QCPAbstractPlottable::setAntialiasedFill(bool enabled)
{
  mAntialiasedFill = enabled;
}

void QCPAbstractPlottable::setAntialiasedScatters(bool enabled)
{
  mAntialiasedScatters = enabled;
}

void QCPAbstractPlottable::setAntialiasedErrorBars(bool enabled)
{
  mAntialiasedErrorBars = enabled;
}

void QCPAbstractPlottable::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPAbstractPlottable::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPAbstractPlottable::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPAbstractPlottable::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

void QCPAbstractPlottable::setKeyAxis(QCPAxis *axis)
{
  mKeyAxis = axis;
}

void QCPAbstractPlottable::setValueAxis(QCPAxis *axis)
{
  mValueAxis = axis;
}

void QCPAbstractPlottable::setSelectable(bool selectable)
{
  if (mSelectable != selectable)
  {
    mSelectable = selectable;
    Q_EMIT selectableChanged(mSelectable);
  }
}

void QCPAbstractPlottable::setSelected(bool selected)
{
  if (mSelected != selected)
  {
    mSelected = selected;
    Q_EMIT selectionChanged(mSelected);
  }
}

void QCPAbstractPlottable::rescaleAxes(bool onlyEnlarge) const
{
  rescaleKeyAxis(onlyEnlarge);
  rescaleValueAxis(onlyEnlarge);
}

void QCPAbstractPlottable::rescaleKeyAxis(bool onlyEnlarge) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  if (!keyAxis) { qDebug() << Q_FUNC_INFO << "invalid key axis"; return; }

  SignDomain signDomain = sdBoth;
  if (keyAxis->scaleType() == QCPAxis::stLogarithmic)
    signDomain = (keyAxis->range().upper < 0 ? sdNegative : sdPositive);

  bool foundRange;
  QCPRange newRange = getKeyRange(foundRange, signDomain);
  if (foundRange)
  {
    if (onlyEnlarge)
      newRange.expand(keyAxis->range());
    if (!QCPRange::validRange(newRange))
    {
      double center = (newRange.lower+newRange.upper)*0.5;
      if (keyAxis->scaleType() == QCPAxis::stLinear)
      {
        newRange.lower = center-keyAxis->range().size()/2.0;
        newRange.upper = center+keyAxis->range().size()/2.0;
      } else
      {
        newRange.lower = center/qSqrt(keyAxis->range().upper/keyAxis->range().lower);
        newRange.upper = center*qSqrt(keyAxis->range().upper/keyAxis->range().lower);
      }
    }
    keyAxis->setRange(newRange);
  }
}

void QCPAbstractPlottable::rescaleValueAxis(bool onlyEnlarge) const
{
  QCPAxis *valueAxis = mValueAxis.data();
  if (!valueAxis) { qDebug() << Q_FUNC_INFO << "invalid value axis"; return; }

  SignDomain signDomain = sdBoth;
  if (valueAxis->scaleType() == QCPAxis::stLogarithmic)
    signDomain = (valueAxis->range().upper < 0 ? sdNegative : sdPositive);

  bool foundRange;
  QCPRange newRange = getValueRange(foundRange, signDomain);
  if (foundRange)
  {
    if (onlyEnlarge)
      newRange.expand(valueAxis->range());
    if (!QCPRange::validRange(newRange))
    {
      double center = (newRange.lower+newRange.upper)*0.5;
      if (valueAxis->scaleType() == QCPAxis::stLinear)
      {
        newRange.lower = center-valueAxis->range().size()/2.0;
        newRange.upper = center+valueAxis->range().size()/2.0;
      } else
      {
        newRange.lower = center/qSqrt(valueAxis->range().upper/valueAxis->range().lower);
        newRange.upper = center*qSqrt(valueAxis->range().upper/valueAxis->range().lower);
      }
    }
    valueAxis->setRange(newRange);
  }
}

bool QCPAbstractPlottable::addToLegend()
{
  if (!mParentPlot || !mParentPlot->legend)
    return false;

  if (!mParentPlot->legend->hasItemWithPlottable(this))
  {
    mParentPlot->legend->addItem(new QCPPlottableLegendItem(mParentPlot->legend, this));
    return true;
  } else
    return false;
}

bool QCPAbstractPlottable::removeFromLegend() const
{
  if (!mParentPlot->legend)
    return false;

  if (QCPPlottableLegendItem *lip = mParentPlot->legend->itemWithPlottable(this))
    return mParentPlot->legend->removeItem(lip);
  else
    return false;
}

QRect QCPAbstractPlottable::clipRect() const
{
  if (mKeyAxis && mValueAxis)
    return mKeyAxis.data()->axisRect()->rect() & mValueAxis.data()->axisRect()->rect();
  else
    return QRect();
}

QCP::Interaction QCPAbstractPlottable::selectionCategory() const
{
  return QCP::iSelectPlottables;
}

void QCPAbstractPlottable::coordsToPixels(double key, double value, double &x, double &y) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  if (keyAxis->orientation() == Qt::Horizontal)
  {
    x = keyAxis->coordToPixel(key);
    y = valueAxis->coordToPixel(value);
  } else
  {
    y = keyAxis->coordToPixel(key);
    x = valueAxis->coordToPixel(value);
  }
}

const QPointF QCPAbstractPlottable::coordsToPixels(double key, double value) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return QPointF(); }

  if (keyAxis->orientation() == Qt::Horizontal)
    return QPointF(keyAxis->coordToPixel(key), valueAxis->coordToPixel(value));
  else
    return QPointF(valueAxis->coordToPixel(value), keyAxis->coordToPixel(key));
}

void QCPAbstractPlottable::pixelsToCoords(double x, double y, double &key, double &value) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  if (keyAxis->orientation() == Qt::Horizontal)
  {
    key = keyAxis->pixelToCoord(x);
    value = valueAxis->pixelToCoord(y);
  } else
  {
    key = keyAxis->pixelToCoord(y);
    value = valueAxis->pixelToCoord(x);
  }
}

void QCPAbstractPlottable::pixelsToCoords(const QPointF &pixelPos, double &key, double &value) const
{
  pixelsToCoords(pixelPos.x(), pixelPos.y(), key, value);
}

QPen QCPAbstractPlottable::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPAbstractPlottable::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}

void QCPAbstractPlottable::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aePlottables);
}

void QCPAbstractPlottable::applyFillAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiasedFill, QCP::aeFills);
}

void QCPAbstractPlottable::applyScattersAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiasedScatters, QCP::aeScatters);
}

void QCPAbstractPlottable::applyErrorBarsAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiasedErrorBars, QCP::aeErrorBars);
}

double QCPAbstractPlottable::distSqrToLine(const QPointF &start, const QPointF &end, const QPointF &point) const
{
  QVector2D a(start);
  QVector2D b(end);
  QVector2D p(point);
  QVector2D v(b-a);

  double vLengthSqr = v.lengthSquared();
  if (!qFuzzyIsNull(vLengthSqr))
  {
    double mu = QVector2D::dotProduct(p-a, v)/vLengthSqr;
    if (mu < 0)
      return (a-p).lengthSquared();
    else if (mu > 1)
      return (b-p).lengthSquared();
    else
      return ((a + mu*v)-p).lengthSquared();
  } else
    return (a-p).lengthSquared();
}

void QCPAbstractPlottable::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  Q_UNUSED(details)
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(additive ? !mSelected : true);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

void QCPAbstractPlottable::deselectEvent(bool *selectionStateChanged)
{
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(false);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

QCPItemAnchor::QCPItemAnchor(QCustomPlot *parentPlot, QCPAbstractItem *parentItem, const QString name, int anchorId) :
  mName(name),
  mParentPlot(parentPlot),
  mParentItem(parentItem),
  mAnchorId(anchorId)
{
}

QCPItemAnchor::~QCPItemAnchor()
{

  QList<QCPItemPosition*> currentChildren(mChildren.toList());
  for (int i=0; i<currentChildren.size(); ++i)
    currentChildren.at(i)->setParentAnchor(0);
}

QPointF QCPItemAnchor::pixelPoint() const
{
  if (mParentItem)
  {
    if (mAnchorId > -1)
    {
      return mParentItem->anchorPixelPoint(mAnchorId);
    } else
    {
      qDebug() << Q_FUNC_INFO << "no valid anchor id set:" << mAnchorId;
      return QPointF();
    }
  } else
  {
    qDebug() << Q_FUNC_INFO << "no parent item set";
    return QPointF();
  }
}

void QCPItemAnchor::addChild(QCPItemPosition *pos)
{
  if (!mChildren.contains(pos))
    mChildren.insert(pos);
  else
    qDebug() << Q_FUNC_INFO << "provided pos is child already" << reinterpret_cast<quintptr>(pos);
}

void QCPItemAnchor::removeChild(QCPItemPosition *pos)
{
  if (!mChildren.remove(pos))
    qDebug() << Q_FUNC_INFO << "provided pos isn't child" << reinterpret_cast<quintptr>(pos);
}

QCPItemPosition::QCPItemPosition(QCustomPlot *parentPlot, QCPAbstractItem *parentItem, const QString name) :
  QCPItemAnchor(parentPlot, parentItem, name),
  mPositionType(ptAbsolute),
  mKey(0),
  mValue(0),
  mParentAnchor(0)
{
}

QCPItemPosition::~QCPItemPosition()
{

  QList<QCPItemPosition*> currentChildren(mChildren.toList());
  for (int i=0; i<currentChildren.size(); ++i)
    currentChildren.at(i)->setParentAnchor(0);

  if (mParentAnchor)
    mParentAnchor->removeChild(this);
}

QCPAxisRect *QCPItemPosition::axisRect() const
{
  return mAxisRect.data();
}

void QCPItemPosition::setType(QCPItemPosition::PositionType type)
{
  if (mPositionType != type)
  {

    bool recoverPixelPosition = true;
    if ((mPositionType == ptPlotCoords || type == ptPlotCoords) && (!mKeyAxis || !mValueAxis))
      recoverPixelPosition = false;
    if ((mPositionType == ptAxisRectRatio || type == ptAxisRectRatio) && (!mAxisRect))
      recoverPixelPosition = false;

    QPointF pixelP;
    if (recoverPixelPosition)
      pixelP = pixelPoint();

    mPositionType = type;

    if (recoverPixelPosition)
      setPixelPoint(pixelP);
  }
}

bool QCPItemPosition::setParentAnchor(QCPItemAnchor *parentAnchor, bool keepPixelPosition)
{

  if (parentAnchor == this)
  {
    qDebug() << Q_FUNC_INFO << "can't set self as parent anchor" << reinterpret_cast<quintptr>(parentAnchor);
    return false;
  }

  QCPItemAnchor *currentParent = parentAnchor;
  while (currentParent)
  {
    if (QCPItemPosition *currentParentPos = currentParent->toQCPItemPosition())
    {

      if (currentParentPos == this)
      {
        qDebug() << Q_FUNC_INFO << "can't create recursive parent-child-relationship" << reinterpret_cast<quintptr>(parentAnchor);
        return false;
      }
      currentParent = currentParentPos->mParentAnchor;
    } else
    {

      if (currentParent->mParentItem == mParentItem)
      {
        qDebug() << Q_FUNC_INFO << "can't set parent to be an anchor which itself depends on this position" << reinterpret_cast<quintptr>(parentAnchor);
        return false;
      }
      break;
    }
  }

  if (!mParentAnchor && mPositionType == ptPlotCoords)
    setType(ptAbsolute);

  QPointF pixelP;
  if (keepPixelPosition)
    pixelP = pixelPoint();

  if (mParentAnchor)
    mParentAnchor->removeChild(this);

  if (parentAnchor)
    parentAnchor->addChild(this);
  mParentAnchor = parentAnchor;

  if (keepPixelPosition)
    setPixelPoint(pixelP);
  else
    setCoords(0, 0);
  return true;
}

void QCPItemPosition::setCoords(double key, double value)
{
  mKey = key;
  mValue = value;
}

void QCPItemPosition::setCoords(const QPointF &pos)
{
  setCoords(pos.x(), pos.y());
}

QPointF QCPItemPosition::pixelPoint() const
{
  switch (mPositionType)
  {
    case ptAbsolute:
    {
      if (mParentAnchor)
        return QPointF(mKey, mValue) + mParentAnchor->pixelPoint();
      else
        return QPointF(mKey, mValue);
    }

    case ptViewportRatio:
    {
      if (mParentAnchor)
      {
        return QPointF(mKey*mParentPlot->viewport().width(),
                       mValue*mParentPlot->viewport().height()) + mParentAnchor->pixelPoint();
      } else
      {
        return QPointF(mKey*mParentPlot->viewport().width(),
                       mValue*mParentPlot->viewport().height()) + mParentPlot->viewport().topLeft();
      }
    }

    case ptAxisRectRatio:
    {
      if (mAxisRect)
      {
        if (mParentAnchor)
        {
          return QPointF(mKey*mAxisRect.data()->width(),
                         mValue*mAxisRect.data()->height()) + mParentAnchor->pixelPoint();
        } else
        {
          return QPointF(mKey*mAxisRect.data()->width(),
                       mValue*mAxisRect.data()->height()) + mAxisRect.data()->topLeft();
        }
      } else
      {
        qDebug() << Q_FUNC_INFO << "No axis rect defined";
        return QPointF(mKey, mValue);
      }
    }

    case ptPlotCoords:
    {
      double x, y;
      if (mKeyAxis && mValueAxis)
      {

        if (mKeyAxis.data()->orientation() == Qt::Horizontal)
        {
          x = mKeyAxis.data()->coordToPixel(mKey);
          y = mValueAxis.data()->coordToPixel(mValue);
        } else
        {
          y = mKeyAxis.data()->coordToPixel(mKey);
          x = mValueAxis.data()->coordToPixel(mValue);
        }
      } else if (mKeyAxis)
      {

        if (mKeyAxis.data()->orientation() == Qt::Horizontal)
        {
          x = mKeyAxis.data()->coordToPixel(mKey);
          y = mValue;
        } else
        {
          y = mKeyAxis.data()->coordToPixel(mKey);
          x = mValue;
        }
      } else if (mValueAxis)
      {

        if (mValueAxis.data()->orientation() == Qt::Horizontal)
        {
          x = mValueAxis.data()->coordToPixel(mValue);
          y = mKey;
        } else
        {
          y = mValueAxis.data()->coordToPixel(mValue);
          x = mKey;
        }
      } else
      {

        qDebug() << Q_FUNC_INFO << "No axes defined";
        x = mKey;
        y = mValue;
      }
      return QPointF(x, y);
    }
  }
  return QPointF();
}

void QCPItemPosition::setAxes(QCPAxis *keyAxis, QCPAxis *valueAxis)
{
  mKeyAxis = keyAxis;
  mValueAxis = valueAxis;
}

void QCPItemPosition::setAxisRect(QCPAxisRect *axisRect)
{
  mAxisRect = axisRect;
}

void QCPItemPosition::setPixelPoint(const QPointF &pixelPoint)
{
  switch (mPositionType)
  {
    case ptAbsolute:
    {
      if (mParentAnchor)
        setCoords(pixelPoint-mParentAnchor->pixelPoint());
      else
        setCoords(pixelPoint);
      break;
    }

    case ptViewportRatio:
    {
      if (mParentAnchor)
      {
        QPointF p(pixelPoint-mParentAnchor->pixelPoint());
        p.rx() /= (double)mParentPlot->viewport().width();
        p.ry() /= (double)mParentPlot->viewport().height();
        setCoords(p);
      } else
      {
        QPointF p(pixelPoint-mParentPlot->viewport().topLeft());
        p.rx() /= (double)mParentPlot->viewport().width();
        p.ry() /= (double)mParentPlot->viewport().height();
        setCoords(p);
      }
      break;
    }

    case ptAxisRectRatio:
    {
      if (mAxisRect)
      {
        if (mParentAnchor)
        {
          QPointF p(pixelPoint-mParentAnchor->pixelPoint());
          p.rx() /= (double)mAxisRect.data()->width();
          p.ry() /= (double)mAxisRect.data()->height();
          setCoords(p);
        } else
        {
          QPointF p(pixelPoint-mAxisRect.data()->topLeft());
          p.rx() /= (double)mAxisRect.data()->width();
          p.ry() /= (double)mAxisRect.data()->height();
          setCoords(p);
        }
      } else
      {
        qDebug() << Q_FUNC_INFO << "No axis rect defined";
        setCoords(pixelPoint);
      }
      break;
    }

    case ptPlotCoords:
    {
      double newKey, newValue;
      if (mKeyAxis && mValueAxis)
      {

        if (mKeyAxis.data()->orientation() == Qt::Horizontal)
        {
          newKey = mKeyAxis.data()->pixelToCoord(pixelPoint.x());
          newValue = mValueAxis.data()->pixelToCoord(pixelPoint.y());
        } else
        {
          newKey = mKeyAxis.data()->pixelToCoord(pixelPoint.y());
          newValue = mValueAxis.data()->pixelToCoord(pixelPoint.x());
        }
      } else if (mKeyAxis)
      {

        if (mKeyAxis.data()->orientation() == Qt::Horizontal)
        {
          newKey = mKeyAxis.data()->pixelToCoord(pixelPoint.x());
          newValue = pixelPoint.y();
        } else
        {
          newKey = mKeyAxis.data()->pixelToCoord(pixelPoint.y());
          newValue = pixelPoint.x();
        }
      } else if (mValueAxis)
      {

        if (mValueAxis.data()->orientation() == Qt::Horizontal)
        {
          newKey = pixelPoint.y();
          newValue = mValueAxis.data()->pixelToCoord(pixelPoint.x());
        } else
        {
          newKey = pixelPoint.x();
          newValue = mValueAxis.data()->pixelToCoord(pixelPoint.y());
        }
      } else
      {

        qDebug() << Q_FUNC_INFO << "No axes defined";
        newKey = pixelPoint.x();
        newValue = pixelPoint.y();
      }
      setCoords(newKey, newValue);
      break;
    }
  }
}

QCPAbstractItem::QCPAbstractItem(QCustomPlot *parentPlot) :
  QCPLayerable(parentPlot),
  mClipToAxisRect(false),
  mSelectable(true),
  mSelected(false)
{
  QList<QCPAxisRect*> rects = parentPlot->axisRects();
  if (rects.size() > 0)
  {
    setClipToAxisRect(true);
    setClipAxisRect(rects.first());
  }
}

QCPAbstractItem::~QCPAbstractItem()
{

  qDeleteAll(mAnchors);
}

QCPAxisRect *QCPAbstractItem::clipAxisRect() const
{
  return mClipAxisRect.data();
}

void QCPAbstractItem::setClipToAxisRect(bool clip)
{
  mClipToAxisRect = clip;
  if (mClipToAxisRect)
    setParentLayerable(mClipAxisRect.data());
}

void QCPAbstractItem::setClipAxisRect(QCPAxisRect *rect)
{
  mClipAxisRect = rect;
  if (mClipToAxisRect)
    setParentLayerable(mClipAxisRect.data());
}

void QCPAbstractItem::setSelectable(bool selectable)
{
  if (mSelectable != selectable)
  {
    mSelectable = selectable;
    Q_EMIT selectableChanged(mSelectable);
  }
}

void QCPAbstractItem::setSelected(bool selected)
{
  if (mSelected != selected)
  {
    mSelected = selected;
    Q_EMIT selectionChanged(mSelected);
  }
}

QCPItemPosition *QCPAbstractItem::position(const QString &name) const
{
  for (int i=0; i<mPositions.size(); ++i)
  {
    if (mPositions.at(i)->name() == name)
      return mPositions.at(i);
  }
  qDebug() << Q_FUNC_INFO << "position with name not found:" << name;
  return 0;
}

QCPItemAnchor *QCPAbstractItem::anchor(const QString &name) const
{
  for (int i=0; i<mAnchors.size(); ++i)
  {
    if (mAnchors.at(i)->name() == name)
      return mAnchors.at(i);
  }
  qDebug() << Q_FUNC_INFO << "anchor with name not found:" << name;
  return 0;
}

bool QCPAbstractItem::hasAnchor(const QString &name) const
{
  for (int i=0; i<mAnchors.size(); ++i)
  {
    if (mAnchors.at(i)->name() == name)
      return true;
  }
  return false;
}

QRect QCPAbstractItem::clipRect() const
{
  if (mClipToAxisRect && mClipAxisRect)
    return mClipAxisRect.data()->rect();
  else
    return mParentPlot->viewport();
}

void QCPAbstractItem::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeItems);
}

double QCPAbstractItem::distSqrToLine(const QPointF &start, const QPointF &end, const QPointF &point) const
{
  QVector2D a(start);
  QVector2D b(end);
  QVector2D p(point);
  QVector2D v(b-a);

  double vLengthSqr = v.lengthSquared();
  if (!qFuzzyIsNull(vLengthSqr))
  {
    double mu = QVector2D::dotProduct(p-a, v)/vLengthSqr;
    if (mu < 0)
      return (a-p).lengthSquared();
    else if (mu > 1)
      return (b-p).lengthSquared();
    else
      return ((a + mu*v)-p).lengthSquared();
  } else
    return (a-p).lengthSquared();
}

double QCPAbstractItem::rectSelectTest(const QRectF &rect, const QPointF &pos, bool filledRect) const
{
  double result = -1;

  QList<QLineF> lines;
  lines << QLineF(rect.topLeft(), rect.topRight()) << QLineF(rect.bottomLeft(), rect.bottomRight())
        << QLineF(rect.topLeft(), rect.bottomLeft()) << QLineF(rect.topRight(), rect.bottomRight());
  double minDistSqr = std::numeric_limits<double>::max();
  for (int i=0; i<lines.size(); ++i)
  {
    double distSqr = distSqrToLine(lines.at(i).p1(), lines.at(i).p2(), pos);
    if (distSqr < minDistSqr)
      minDistSqr = distSqr;
  }
  result = qSqrt(minDistSqr);

  if (filledRect && result > mParentPlot->selectionTolerance()*0.99)
  {
    if (rect.contains(pos))
      result = mParentPlot->selectionTolerance()*0.99;
  }
  return result;
}

QPointF QCPAbstractItem::anchorPixelPoint(int anchorId) const
{
  qDebug() << Q_FUNC_INFO << "called on item which shouldn't have any anchors (this method not reimplemented). anchorId" << anchorId;
  return QPointF();
}

QCPItemPosition *QCPAbstractItem::createPosition(const QString &name)
{
  if (hasAnchor(name))
    qDebug() << Q_FUNC_INFO << "anchor/position with name exists already:" << name;
  QCPItemPosition *newPosition = new QCPItemPosition(mParentPlot, this, name);
  mPositions.append(newPosition);
  mAnchors.append(newPosition);
  newPosition->setAxes(mParentPlot->xAxis, mParentPlot->yAxis);
  newPosition->setType(QCPItemPosition::ptPlotCoords);
  if (mParentPlot->axisRect())
    newPosition->setAxisRect(mParentPlot->axisRect());
  newPosition->setCoords(0, 0);
  return newPosition;
}

QCPItemAnchor *QCPAbstractItem::createAnchor(const QString &name, int anchorId)
{
  if (hasAnchor(name))
    qDebug() << Q_FUNC_INFO << "anchor/position with name exists already:" << name;
  QCPItemAnchor *newAnchor = new QCPItemAnchor(mParentPlot, this, name, anchorId);
  mAnchors.append(newAnchor);
  return newAnchor;
}

void QCPAbstractItem::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  Q_UNUSED(details)
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(additive ? !mSelected : true);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

void QCPAbstractItem::deselectEvent(bool *selectionStateChanged)
{
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(false);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

QCP::Interaction QCPAbstractItem::selectionCategory() const
{
  return QCP::iSelectItems;
}

QCustomPlot::QCustomPlot(QWidget *parent) :
  QWidget(parent),
  xAxis(0),
  yAxis(0),
  xAxis2(0),
  yAxis2(0),
  legend(0),
  mPlotLayout(0),
  mAutoAddPlottableToLegend(true),
  mAntialiasedElements(QCP::aeNone),
  mNotAntialiasedElements(QCP::aeNone),
  mInteractions(0),
  mSelectionTolerance(8),
  mNoAntialiasingOnDrag(false),
  mBackgroundBrush(Qt::white, Qt::SolidPattern),
  mBackgroundScaled(true),
  mBackgroundScaledMode(Qt::KeepAspectRatioByExpanding),
  mCurrentLayer(0),
  mPlottingHints(QCP::phCacheLabels|QCP::phForceRepaint),
  mMultiSelectModifier(Qt::ControlModifier),
  mPaintBuffer(size()),
  mMouseEventElement(0),
  mReplotting(false)
{
  setAttribute(Qt::WA_NoMousePropagation);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setMouseTracking(true);
  QLocale currentLocale = locale();
  currentLocale.setNumberOptions(QLocale::OmitGroupSeparator);
  setLocale(currentLocale);

  mLayers.append(new QCPLayer(this, "background"));
  mLayers.append(new QCPLayer(this, "grid"));
  mLayers.append(new QCPLayer(this, "main"));
  mLayers.append(new QCPLayer(this, "axes"));
  mLayers.append(new QCPLayer(this, "legend"));
  updateLayerIndices();
  setCurrentLayer("main");

  mPlotLayout = new QCPLayoutGrid;
  mPlotLayout->initializeParentPlot(this);
  mPlotLayout->setParent(this);
  mPlotLayout->setLayer("main");
  QCPAxisRect *defaultAxisRect = new QCPAxisRect(this, true);
  mPlotLayout->addElement(0, 0, defaultAxisRect);
  xAxis = defaultAxisRect->axis(QCPAxis::atBottom);
  yAxis = defaultAxisRect->axis(QCPAxis::atLeft);
  xAxis2 = defaultAxisRect->axis(QCPAxis::atTop);
  yAxis2 = defaultAxisRect->axis(QCPAxis::atRight);
  legend = new QCPLegend;
  legend->setVisible(false);
  defaultAxisRect->insetLayout()->addElement(legend, Qt::AlignRight|Qt::AlignTop);
  defaultAxisRect->insetLayout()->setMargins(QMargins(12, 12, 12, 12));

  defaultAxisRect->setLayer("background");
  xAxis->setLayer("axes");
  yAxis->setLayer("axes");
  xAxis2->setLayer("axes");
  yAxis2->setLayer("axes");
  xAxis->grid()->setLayer("grid");
  yAxis->grid()->setLayer("grid");
  xAxis2->grid()->setLayer("grid");
  yAxis2->grid()->setLayer("grid");
  legend->setLayer("legend");

  setViewport(rect());

  replot();
}

QCustomPlot::~QCustomPlot()
{
  clearPlottables();
  clearItems();

  if (mPlotLayout)
  {
    delete mPlotLayout;
    mPlotLayout = 0;
  }

  mCurrentLayer = 0;
  qDeleteAll(mLayers);
  mLayers.clear();
}

void QCustomPlot::setAntialiasedElements(const QCP::AntialiasedElements &antialiasedElements)
{
  mAntialiasedElements = antialiasedElements;

  if ((mNotAntialiasedElements & mAntialiasedElements) != 0)
    mNotAntialiasedElements |= ~mAntialiasedElements;
}

void QCustomPlot::setAntialiasedElement(QCP::AntialiasedElement antialiasedElement, bool enabled)
{
  if (!enabled && mAntialiasedElements.testFlag(antialiasedElement))
    mAntialiasedElements &= ~antialiasedElement;
  else if (enabled && !mAntialiasedElements.testFlag(antialiasedElement))
    mAntialiasedElements |= antialiasedElement;

  if ((mNotAntialiasedElements & mAntialiasedElements) != 0)
    mNotAntialiasedElements |= ~mAntialiasedElements;
}

void QCustomPlot::setNotAntialiasedElements(const QCP::AntialiasedElements &notAntialiasedElements)
{
  mNotAntialiasedElements = notAntialiasedElements;

  if ((mNotAntialiasedElements & mAntialiasedElements) != 0)
    mAntialiasedElements |= ~mNotAntialiasedElements;
}

void QCustomPlot::setNotAntialiasedElement(QCP::AntialiasedElement notAntialiasedElement, bool enabled)
{
  if (!enabled && mNotAntialiasedElements.testFlag(notAntialiasedElement))
    mNotAntialiasedElements &= ~notAntialiasedElement;
  else if (enabled && !mNotAntialiasedElements.testFlag(notAntialiasedElement))
    mNotAntialiasedElements |= notAntialiasedElement;

  if ((mNotAntialiasedElements & mAntialiasedElements) != 0)
    mAntialiasedElements |= ~mNotAntialiasedElements;
}

void QCustomPlot::setAutoAddPlottableToLegend(bool on)
{
  mAutoAddPlottableToLegend = on;
}

void QCustomPlot::setInteractions(const QCP::Interactions &interactions)
{
  mInteractions = interactions;
}

void QCustomPlot::setInteraction(const QCP::Interaction &interaction, bool enabled)
{
  if (!enabled && mInteractions.testFlag(interaction))
    mInteractions &= ~interaction;
  else if (enabled && !mInteractions.testFlag(interaction))
    mInteractions |= interaction;
}

void QCustomPlot::setSelectionTolerance(int pixels)
{
  mSelectionTolerance = pixels;
}

void QCustomPlot::setNoAntialiasingOnDrag(bool enabled)
{
  mNoAntialiasingOnDrag = enabled;
}

void QCustomPlot::setPlottingHints(const QCP::PlottingHints &hints)
{
  mPlottingHints = hints;
}

void QCustomPlot::setPlottingHint(QCP::PlottingHint hint, bool enabled)
{
  QCP::PlottingHints newHints = mPlottingHints;
  if (!enabled)
    newHints &= ~hint;
  else
    newHints |= hint;

  if (newHints != mPlottingHints)
    setPlottingHints(newHints);
}

void QCustomPlot::setMultiSelectModifier(Qt::KeyboardModifier modifier)
{
  mMultiSelectModifier = modifier;
}

void QCustomPlot::setViewport(const QRect &rect)
{
  mViewport = rect;
  if (mPlotLayout)
    mPlotLayout->setOuterRect(mViewport);
}

void QCustomPlot::setBackground(const QPixmap &pm)
{
  mBackgroundPixmap = pm;
  mScaledBackgroundPixmap = QPixmap();
}

void QCustomPlot::setBackground(const QBrush &brush)
{
  mBackgroundBrush = brush;
}

void QCustomPlot::setBackground(const QPixmap &pm, bool scaled, Qt::AspectRatioMode mode)
{
  mBackgroundPixmap = pm;
  mScaledBackgroundPixmap = QPixmap();
  mBackgroundScaled = scaled;
  mBackgroundScaledMode = mode;
}

void QCustomPlot::setBackgroundScaled(bool scaled)
{
  mBackgroundScaled = scaled;
}

void QCustomPlot::setBackgroundScaledMode(Qt::AspectRatioMode mode)
{
  mBackgroundScaledMode = mode;
}

QCPAbstractPlottable *QCustomPlot::plottable(int index)
{
  if (index >= 0 && index < mPlottables.size())
  {
    return mPlottables.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return 0;
  }
}

QCPAbstractPlottable *QCustomPlot::plottable()
{
  if (!mPlottables.isEmpty())
  {
    return mPlottables.last();
  } else
    return 0;
}

bool QCustomPlot::addPlottable(QCPAbstractPlottable *plottable)
{
  if (mPlottables.contains(plottable))
  {
    qDebug() << Q_FUNC_INFO << "plottable already added to this QCustomPlot:" << reinterpret_cast<quintptr>(plottable);
    return false;
  }
  if (plottable->parentPlot() != this)
  {
    qDebug() << Q_FUNC_INFO << "plottable not created with this QCustomPlot as parent:" << reinterpret_cast<quintptr>(plottable);
    return false;
  }

  mPlottables.append(plottable);

  if (mAutoAddPlottableToLegend)
    plottable->addToLegend();

  if (QCPGraph *graph = qobject_cast<QCPGraph*>(plottable))
    mGraphs.append(graph);
  if (!plottable->layer())
    plottable->setLayer(currentLayer());
  return true;
}

bool QCustomPlot::removePlottable(QCPAbstractPlottable *plottable)
{
  if (!mPlottables.contains(plottable))
  {
    qDebug() << Q_FUNC_INFO << "plottable not in list:" << reinterpret_cast<quintptr>(plottable);
    return false;
  }

  plottable->removeFromLegend();

  if (QCPGraph *graph = qobject_cast<QCPGraph*>(plottable))
    mGraphs.removeOne(graph);

  delete plottable;
  mPlottables.removeOne(plottable);
  return true;
}

bool QCustomPlot::removePlottable(int index)
{
  if (index >= 0 && index < mPlottables.size())
    return removePlottable(mPlottables[index]);
  else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return false;
  }
}

int QCustomPlot::clearPlottables()
{
  int c = mPlottables.size();
  for (int i=c-1; i >= 0; --i)
    removePlottable(mPlottables[i]);
  return c;
}

int QCustomPlot::plottableCount() const
{
  return mPlottables.size();
}

QList<QCPAbstractPlottable*> QCustomPlot::selectedPlottables() const
{
  QList<QCPAbstractPlottable*> result;
  Q_FOREACH (QCPAbstractPlottable *plottable, mPlottables)
  {
    if (plottable->selected())
      result.append(plottable);
  }
  return result;
}

QCPAbstractPlottable *QCustomPlot::plottableAt(const QPointF &pos, bool onlySelectable) const
{
  QCPAbstractPlottable *resultPlottable = 0;
  double resultDistance = mSelectionTolerance;

  Q_FOREACH (QCPAbstractPlottable *plottable, mPlottables)
  {
    if (onlySelectable && !plottable->selectable())
      continue;
    if ((plottable->keyAxis()->axisRect()->rect() & plottable->valueAxis()->axisRect()->rect()).contains(pos.toPoint()))
    {
      double currentDistance = plottable->selectTest(pos, false);
      if (currentDistance >= 0 && currentDistance < resultDistance)
      {
        resultPlottable = plottable;
        resultDistance = currentDistance;
      }
    }
  }

  return resultPlottable;
}

bool QCustomPlot::hasPlottable(QCPAbstractPlottable *plottable) const
{
  return mPlottables.contains(plottable);
}

QCPGraph *QCustomPlot::graph(int index) const
{
  if (index >= 0 && index < mGraphs.size())
  {
    return mGraphs.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return 0;
  }
}

QCPGraph *QCustomPlot::graph() const
{
  if (!mGraphs.isEmpty())
  {
    return mGraphs.last();
  } else
    return 0;
}

QCPGraph *QCustomPlot::addGraph(QCPAxis *keyAxis, QCPAxis *valueAxis)
{
  if (!keyAxis) keyAxis = xAxis;
  if (!valueAxis) valueAxis = yAxis;
  if (!keyAxis || !valueAxis)
  {
    qDebug() << Q_FUNC_INFO << "can't use default QCustomPlot xAxis or yAxis, because at least one is invalid (has been deleted)";
    return 0;
  }
  if (keyAxis->parentPlot() != this || valueAxis->parentPlot() != this)
  {
    qDebug() << Q_FUNC_INFO << "passed keyAxis or valueAxis doesn't have this QCustomPlot as parent";
    return 0;
  }

  QCPGraph *newGraph = new QCPGraph(keyAxis, valueAxis);
  if (addPlottable(newGraph))
  {
    newGraph->setName("Graph "+QString::number(mGraphs.size()));
    return newGraph;
  } else
  {
    delete newGraph;
    return 0;
  }
}

bool QCustomPlot::removeGraph(QCPGraph *graph)
{
  return removePlottable(graph);
}

bool QCustomPlot::removeGraph(int index)
{
  if (index >= 0 && index < mGraphs.size())
    return removeGraph(mGraphs[index]);
  else
    return false;
}

int QCustomPlot::clearGraphs()
{
  int c = mGraphs.size();
  for (int i=c-1; i >= 0; --i)
    removeGraph(mGraphs[i]);
  return c;
}

int QCustomPlot::graphCount() const
{
  return mGraphs.size();
}

QList<QCPGraph*> QCustomPlot::selectedGraphs() const
{
  QList<QCPGraph*> result;
  Q_FOREACH (QCPGraph *graph, mGraphs)
  {
    if (graph->selected())
      result.append(graph);
  }
  return result;
}

QCPAbstractItem *QCustomPlot::item(int index) const
{
  if (index >= 0 && index < mItems.size())
  {
    return mItems.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return 0;
  }
}

QCPAbstractItem *QCustomPlot::item() const
{
  if (!mItems.isEmpty())
  {
    return mItems.last();
  } else
    return 0;
}

bool QCustomPlot::addItem(QCPAbstractItem *item)
{
  if (!mItems.contains(item) && item->parentPlot() == this)
  {
    mItems.append(item);
    return true;
  } else
  {
    qDebug() << Q_FUNC_INFO << "item either already in list or not created with this QCustomPlot as parent:" << reinterpret_cast<quintptr>(item);
    return false;
  }
}

bool QCustomPlot::removeItem(QCPAbstractItem *item)
{
  if (mItems.contains(item))
  {
    delete item;
    mItems.removeOne(item);
    return true;
  } else
  {
    qDebug() << Q_FUNC_INFO << "item not in list:" << reinterpret_cast<quintptr>(item);
    return false;
  }
}

bool QCustomPlot::removeItem(int index)
{
  if (index >= 0 && index < mItems.size())
    return removeItem(mItems[index]);
  else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return false;
  }
}

int QCustomPlot::clearItems()
{
  int c = mItems.size();
  for (int i=c-1; i >= 0; --i)
    removeItem(mItems[i]);
  return c;
}

int QCustomPlot::itemCount() const
{
  return mItems.size();
}

QList<QCPAbstractItem*> QCustomPlot::selectedItems() const
{
  QList<QCPAbstractItem*> result;
  Q_FOREACH (QCPAbstractItem *item, mItems)
  {
    if (item->selected())
      result.append(item);
  }
  return result;
}

QCPAbstractItem *QCustomPlot::itemAt(const QPointF &pos, bool onlySelectable) const
{
  QCPAbstractItem *resultItem = 0;
  double resultDistance = mSelectionTolerance;

  Q_FOREACH (QCPAbstractItem *item, mItems)
  {
    if (onlySelectable && !item->selectable())
      continue;
    if (!item->clipToAxisRect() || item->clipRect().contains(pos.toPoint()))
    {
      double currentDistance = item->selectTest(pos, false);
      if (currentDistance >= 0 && currentDistance < resultDistance)
      {
        resultItem = item;
        resultDistance = currentDistance;
      }
    }
  }

  return resultItem;
}

bool QCustomPlot::hasItem(QCPAbstractItem *item) const
{
  return mItems.contains(item);
}

QCPLayer *QCustomPlot::layer(const QString &name) const
{
  Q_FOREACH (QCPLayer *layer, mLayers)
  {
    if (layer->name() == name)
      return layer;
  }
  return 0;
}

QCPLayer *QCustomPlot::layer(int index) const
{
  if (index >= 0 && index < mLayers.size())
  {
    return mLayers.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
    return 0;
  }
}

QCPLayer *QCustomPlot::currentLayer() const
{
  return mCurrentLayer;
}

bool QCustomPlot::setCurrentLayer(const QString &name)
{
  if (QCPLayer *newCurrentLayer = layer(name))
  {
    return setCurrentLayer(newCurrentLayer);
  } else
  {
    qDebug() << Q_FUNC_INFO << "layer with name doesn't exist:" << name;
    return false;
  }
}

bool QCustomPlot::setCurrentLayer(QCPLayer *layer)
{
  if (!mLayers.contains(layer))
  {
    qDebug() << Q_FUNC_INFO << "layer not a layer of this QCustomPlot:" << reinterpret_cast<quintptr>(layer);
    return false;
  }

  mCurrentLayer = layer;
  return true;
}

int QCustomPlot::layerCount() const
{
  return mLayers.size();
}

bool QCustomPlot::addLayer(const QString &name, QCPLayer *otherLayer, QCustomPlot::LayerInsertMode insertMode)
{
  if (!otherLayer)
    otherLayer = mLayers.last();
  if (!mLayers.contains(otherLayer))
  {
    qDebug() << Q_FUNC_INFO << "otherLayer not a layer of this QCustomPlot:" << reinterpret_cast<quintptr>(otherLayer);
    return false;
  }
  if (layer(name))
  {
    qDebug() << Q_FUNC_INFO << "A layer exists already with the name" << name;
    return false;
  }

  QCPLayer *newLayer = new QCPLayer(this, name);
  mLayers.insert(otherLayer->index() + (insertMode==limAbove ? 1:0), newLayer);
  updateLayerIndices();
  return true;
}

bool QCustomPlot::removeLayer(QCPLayer *layer)
{
  if (!mLayers.contains(layer))
  {
    qDebug() << Q_FUNC_INFO << "layer not a layer of this QCustomPlot:" << reinterpret_cast<quintptr>(layer);
    return false;
  }
  if (mLayers.size() < 2)
  {
    qDebug() << Q_FUNC_INFO << "can't remove last layer";
    return false;
  }

  int removedIndex = layer->index();
  bool isFirstLayer = removedIndex==0;
  QCPLayer *targetLayer = isFirstLayer ? mLayers.at(removedIndex+1) : mLayers.at(removedIndex-1);
  QList<QCPLayerable*> children = layer->children();
  if (isFirstLayer)
  {
    for (int i=children.size()-1; i>=0; --i)
      children.at(i)->moveToLayer(targetLayer, true);
  } else
  {
    for (int i=0; i<children.size(); ++i)
      children.at(i)->moveToLayer(targetLayer, false);
  }

  if (layer == mCurrentLayer)
    setCurrentLayer(targetLayer);

  delete layer;
  mLayers.removeOne(layer);
  updateLayerIndices();
  return true;
}

bool QCustomPlot::moveLayer(QCPLayer *layer, QCPLayer *otherLayer, QCustomPlot::LayerInsertMode insertMode)
{
  if (!mLayers.contains(layer))
  {
    qDebug() << Q_FUNC_INFO << "layer not a layer of this QCustomPlot:" << reinterpret_cast<quintptr>(layer);
    return false;
  }
  if (!mLayers.contains(otherLayer))
  {
    qDebug() << Q_FUNC_INFO << "otherLayer not a layer of this QCustomPlot:" << reinterpret_cast<quintptr>(otherLayer);
    return false;
  }

  mLayers.move(layer->index(), otherLayer->index() + (insertMode==limAbove ? 1:0));
  updateLayerIndices();
  return true;
}

int QCustomPlot::axisRectCount() const
{
  return axisRects().size();
}

QCPAxisRect *QCustomPlot::axisRect(int index) const
{
  const QList<QCPAxisRect*> rectList = axisRects();
  if (index >= 0 && index < rectList.size())
  {
    return rectList.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "invalid axis rect index" << index;
    return 0;
  }
}

QList<QCPAxisRect*> QCustomPlot::axisRects() const
{
  QList<QCPAxisRect*> result;
  QStack<QCPLayoutElement*> elementStack;
  if (mPlotLayout)
    elementStack.push(mPlotLayout);

  while (!elementStack.isEmpty())
  {
    Q_FOREACH (QCPLayoutElement *element, elementStack.pop()->elements(false))
    {
      if (element)
      {
        elementStack.push(element);
        if (QCPAxisRect *ar = qobject_cast<QCPAxisRect*>(element))
          result.append(ar);
      }
    }
  }

  return result;
}

QCPLayoutElement *QCustomPlot::layoutElementAt(const QPointF &pos) const
{
  QCPLayoutElement *currentElement = mPlotLayout;
  bool searchSubElements = true;
  while (searchSubElements && currentElement)
  {
    searchSubElements = false;
    Q_FOREACH (QCPLayoutElement *subElement, currentElement->elements(false))
    {
      if (subElement && subElement->realVisibility() && subElement->selectTest(pos, false) >= 0)
      {
        currentElement = subElement;
        searchSubElements = true;
        break;
      }
    }
  }
  return currentElement;
}

QList<QCPAxis*> QCustomPlot::selectedAxes() const
{
  QList<QCPAxis*> result, allAxes;
  Q_FOREACH (QCPAxisRect *rect, axisRects())
    allAxes << rect->axes();

  Q_FOREACH (QCPAxis *axis, allAxes)
  {
    if (axis->selectedParts() != QCPAxis::spNone)
      result.append(axis);
  }

  return result;
}

QList<QCPLegend*> QCustomPlot::selectedLegends() const
{
  QList<QCPLegend*> result;

  QStack<QCPLayoutElement*> elementStack;
  if (mPlotLayout)
    elementStack.push(mPlotLayout);

  while (!elementStack.isEmpty())
  {
    Q_FOREACH (QCPLayoutElement *subElement, elementStack.pop()->elements(false))
    {
      if (subElement)
      {
        elementStack.push(subElement);
        if (QCPLegend *leg = qobject_cast<QCPLegend*>(subElement))
        {
          if (leg->selectedParts() != QCPLegend::spNone)
            result.append(leg);
        }
      }
    }
  }

  return result;
}

void QCustomPlot::deselectAll()
{
  Q_FOREACH (QCPLayer *layer, mLayers)
  {
    Q_FOREACH (QCPLayerable *layerable, layer->children())
      layerable->deselectEvent(0);
  }
}

void QCustomPlot::replot(QCustomPlot::RefreshPriority refreshPriority)
{
  if (mReplotting)
    return;
  mReplotting = true;
  Q_EMIT beforeReplot();

  mPaintBuffer.fill(mBackgroundBrush.style() == Qt::SolidPattern ? mBackgroundBrush.color() : Qt::transparent);
  QCPPainter painter;
  painter.begin(&mPaintBuffer);
  if (painter.isActive())
  {
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    if (mBackgroundBrush.style() != Qt::SolidPattern && mBackgroundBrush.style() != Qt::NoBrush)
      painter.fillRect(mViewport, mBackgroundBrush);
    draw(&painter);
    painter.end();
    if ((refreshPriority == rpHint && mPlottingHints.testFlag(QCP::phForceRepaint)) || refreshPriority==rpImmediate)
      repaint();
    else
      update();
  } else
    qDebug() << Q_FUNC_INFO << "Couldn't activate painter on buffer";

  Q_EMIT afterReplot();
  mReplotting = false;
}

void QCustomPlot::rescaleAxes(bool onlyVisiblePlottables)
{
  QList<QCPAxis*> allAxes;
  Q_FOREACH (QCPAxisRect *rect, axisRects())
    allAxes << rect->axes();

  Q_FOREACH (QCPAxis *axis, allAxes)
    axis->rescale(onlyVisiblePlottables);
}

bool QCustomPlot::savePdf(const QString &fileName, bool noCosmeticPen, int width, int height, const QString &pdfCreator, const QString &pdfTitle)
{
  bool success = false;
#ifdef QT_NO_PRINTER
  Q_UNUSED(fileName)
  Q_UNUSED(noCosmeticPen)
  Q_UNUSED(width)
  Q_UNUSED(height)
  qDebug() << Q_FUNC_INFO << "Qt was built without printer support (QT_NO_PRINTER). PDF not created.";
#else
  int newWidth, newHeight;
  if (width == 0 || height == 0)
  {
    newWidth = this->width();
    newHeight = this->height();
  } else
  {
    newWidth = width;
    newHeight = height;
  }

  QPrinter printer(QPrinter::ScreenResolution);
  printer.setOutputFileName(fileName);
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setFullPage(true);
  printer.setColorMode(QPrinter::Color);
  printer.printEngine()->setProperty(QPrintEngine::PPK_Creator, pdfCreator);
  printer.printEngine()->setProperty(QPrintEngine::PPK_DocumentName, pdfTitle);
  QRect oldViewport = viewport();
  setViewport(QRect(0, 0, newWidth, newHeight));
  printer.setPaperSize(viewport().size(), QPrinter::DevicePixel);
  QCPPainter printpainter;
  if (printpainter.begin(&printer))
  {
    printpainter.setMode(QCPPainter::pmVectorized);
    printpainter.setMode(QCPPainter::pmNoCaching);
    printpainter.setMode(QCPPainter::pmNonCosmetic, noCosmeticPen);
    printpainter.setWindow(mViewport);
    if (mBackgroundBrush.style() != Qt::NoBrush &&
        mBackgroundBrush.color() != Qt::white &&
        mBackgroundBrush.color() != Qt::transparent &&
        mBackgroundBrush.color().alpha() > 0)
      printpainter.fillRect(viewport(), mBackgroundBrush);
    draw(&printpainter);
    printpainter.end();
    success = true;
  }
  setViewport(oldViewport);
#endif
  return success;
}

bool QCustomPlot::savePng(const QString &fileName, int width, int height, double scale, int quality)
{
  return saveRastered(fileName, width, height, scale, "PNG", quality);
}

bool QCustomPlot::saveJpg(const QString &fileName, int width, int height, double scale, int quality)
{
  return saveRastered(fileName, width, height, scale, "JPG", quality);
}

bool QCustomPlot::saveBmp(const QString &fileName, int width, int height, double scale)
{
  return saveRastered(fileName, width, height, scale, "BMP");
}

QSize QCustomPlot::minimumSizeHint() const
{
  return mPlotLayout->minimumSizeHint();
}

QSize QCustomPlot::sizeHint() const
{
  return mPlotLayout->minimumSizeHint();
}

void QCustomPlot::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  QPainter painter(this);
  painter.drawPixmap(0, 0, mPaintBuffer);
}

void QCustomPlot::resizeEvent(QResizeEvent *event)
{

  mPaintBuffer = QPixmap(event->size());
  setViewport(rect());
  replot(rpQueued);
}

void QCustomPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_EMIT mouseDoubleClick(event);

  QVariant details;
  QCPLayerable *clickedLayerable = layerableAt(event->pos(), false, &details);

  if (QCPAbstractPlottable *ap = qobject_cast<QCPAbstractPlottable*>(clickedLayerable))
    Q_EMIT plottableDoubleClick(ap, event);
  else if (QCPAxis *ax = qobject_cast<QCPAxis*>(clickedLayerable))
    Q_EMIT axisDoubleClick(ax, details.value<QCPAxis::SelectablePart>(), event);
  else if (QCPAbstractItem *ai = qobject_cast<QCPAbstractItem*>(clickedLayerable))
    Q_EMIT itemDoubleClick(ai, event);
  else if (QCPLegend *lg = qobject_cast<QCPLegend*>(clickedLayerable))
    Q_EMIT legendDoubleClick(lg, 0, event);
  else if (QCPAbstractLegendItem *li = qobject_cast<QCPAbstractLegendItem*>(clickedLayerable))
    Q_EMIT legendDoubleClick(li->parentLegend(), li, event);
  else if (QCPPlotTitle *pt = qobject_cast<QCPPlotTitle*>(clickedLayerable))
    Q_EMIT titleDoubleClick(event, pt);

  if (QCPLayoutElement *el = layoutElementAt(event->pos()))
    el->mouseDoubleClickEvent(event);

  if (mMouseEventElement)
  {
    mMouseEventElement->mouseReleaseEvent(event);
    mMouseEventElement = 0;
  }

}

void QCustomPlot::mousePressEvent(QMouseEvent *event)
{
  Q_EMIT mousePress(event);
  mMousePressPos = event->pos();

  mMouseEventElement = layoutElementAt(event->pos());
  if (mMouseEventElement)
    mMouseEventElement->mousePressEvent(event);

  QWidget::mousePressEvent(event);
}

void QCustomPlot::mouseMoveEvent(QMouseEvent *event)
{
  Q_EMIT mouseMove(event);

  if (mMouseEventElement)
    mMouseEventElement->mouseMoveEvent(event);

  QWidget::mouseMoveEvent(event);
}

void QCustomPlot::mouseReleaseEvent(QMouseEvent *event)
{
  Q_EMIT mouseRelease(event);
  bool doReplot = false;

  if ((mMousePressPos-event->pos()).manhattanLength() < 5)
  {
    if (event->button() == Qt::LeftButton)
    {

      QVariant details;
      QCPLayerable *clickedLayerable = layerableAt(event->pos(), true, &details);
      bool selectionStateChanged = false;
      bool additive = mInteractions.testFlag(QCP::iMultiSelect) && event->modifiers().testFlag(mMultiSelectModifier);

      if (!additive)
      {
        Q_FOREACH (QCPLayer *layer, mLayers)
        {
          Q_FOREACH (QCPLayerable *layerable, layer->children())
          {
            if (layerable != clickedLayerable && mInteractions.testFlag(layerable->selectionCategory()))
            {
              bool selChanged = false;
              layerable->deselectEvent(&selChanged);
              selectionStateChanged |= selChanged;
            }
          }
        }
      }
      if (clickedLayerable && mInteractions.testFlag(clickedLayerable->selectionCategory()))
      {

        bool selChanged = false;
        clickedLayerable->selectEvent(event, additive, details, &selChanged);
        selectionStateChanged |= selChanged;
      }
      doReplot = true;
      if (selectionStateChanged)
        Q_EMIT selectionChangedByUser();
    }

    QVariant details;
    QCPLayerable *clickedLayerable = layerableAt(event->pos(), false, &details);
    if (QCPAbstractPlottable *ap = qobject_cast<QCPAbstractPlottable*>(clickedLayerable))
      Q_EMIT plottableClick(ap, event);
    else if (QCPAxis *ax = qobject_cast<QCPAxis*>(clickedLayerable))
      Q_EMIT axisClick(ax, details.value<QCPAxis::SelectablePart>(), event);
    else if (QCPAbstractItem *ai = qobject_cast<QCPAbstractItem*>(clickedLayerable))
      Q_EMIT itemClick(ai, event);
    else if (QCPLegend *lg = qobject_cast<QCPLegend*>(clickedLayerable))
      Q_EMIT legendClick(lg, 0, event);
    else if (QCPAbstractLegendItem *li = qobject_cast<QCPAbstractLegendItem*>(clickedLayerable))
      Q_EMIT legendClick(li->parentLegend(), li, event);
    else if (QCPPlotTitle *pt = qobject_cast<QCPPlotTitle*>(clickedLayerable))
      Q_EMIT titleClick(event, pt);
  }

  if (mMouseEventElement)
  {
    mMouseEventElement->mouseReleaseEvent(event);
    mMouseEventElement = 0;
  }

  if (doReplot || noAntialiasingOnDrag())
    replot();

  QWidget::mouseReleaseEvent(event);
}

void QCustomPlot::wheelEvent(QWheelEvent *event)
{
  Q_EMIT mouseWheel(event);

  if (QCPLayoutElement *el = layoutElementAt(event->pos()))
    el->wheelEvent(event);

  QWidget::wheelEvent(event);
}

void QCustomPlot::draw(QCPPainter *painter)
{

  mPlotLayout->update(QCPLayoutElement::upPreparation);
  mPlotLayout->update(QCPLayoutElement::upMargins);
  mPlotLayout->update(QCPLayoutElement::upLayout);

  drawBackground(painter);

  Q_FOREACH (QCPLayer *layer, mLayers)
  {
    Q_FOREACH (QCPLayerable *child, layer->children())
    {
      if (child->realVisibility())
      {
        painter->save();
        painter->setClipRect(child->clipRect().translated(0, -1));
        child->applyDefaultAntialiasingHint(painter);
        child->draw(painter);
        painter->restore();
      }
    }
  }

}

void QCustomPlot::drawBackground(QCPPainter *painter)
{

  if (!mBackgroundPixmap.isNull())
  {
    if (mBackgroundScaled)
    {

      QSize scaledSize(mBackgroundPixmap.size());
      scaledSize.scale(mViewport.size(), mBackgroundScaledMode);
      if (mScaledBackgroundPixmap.size() != scaledSize)
        mScaledBackgroundPixmap = mBackgroundPixmap.scaled(mViewport.size(), mBackgroundScaledMode, Qt::SmoothTransformation);
      painter->drawPixmap(mViewport.topLeft(), mScaledBackgroundPixmap, QRect(0, 0, mViewport.width(), mViewport.height()) & mScaledBackgroundPixmap.rect());
    } else
    {
      painter->drawPixmap(mViewport.topLeft(), mBackgroundPixmap, QRect(0, 0, mViewport.width(), mViewport.height()));
    }
  }
}

void QCustomPlot::axisRemoved(QCPAxis *axis)
{
  if (xAxis == axis)
    xAxis = 0;
  if (xAxis2 == axis)
    xAxis2 = 0;
  if (yAxis == axis)
    yAxis = 0;
  if (yAxis2 == axis)
    yAxis2 = 0;

}

void QCustomPlot::legendRemoved(QCPLegend *legend)
{
  if (this->legend == legend)
    this->legend = 0;
}

void QCustomPlot::updateLayerIndices() const
{
  for (int i=0; i<mLayers.size(); ++i)
    mLayers.at(i)->mIndex = i;
}

QCPLayerable *QCustomPlot::layerableAt(const QPointF &pos, bool onlySelectable, QVariant *selectionDetails) const
{
  for (int layerIndex=mLayers.size()-1; layerIndex>=0; --layerIndex)
  {
    const QList<QCPLayerable*> layerables = mLayers.at(layerIndex)->children();
    double minimumDistance = selectionTolerance()*1.1;
    QCPLayerable *minimumDistanceLayerable = 0;
    for (int i=layerables.size()-1; i>=0; --i)
    {
      if (!layerables.at(i)->realVisibility())
        continue;
      QVariant details;
      double dist = layerables.at(i)->selectTest(pos, onlySelectable, &details);
      if (dist >= 0 && dist < minimumDistance)
      {
        minimumDistance = dist;
        minimumDistanceLayerable = layerables.at(i);
        if (selectionDetails) *selectionDetails = details;
      }
    }
    if (minimumDistance < selectionTolerance())
      return minimumDistanceLayerable;
  }
  return 0;
}

bool QCustomPlot::saveRastered(const QString &fileName, int width, int height, double scale, const char *format, int quality)
{
  QPixmap buffer = toPixmap(width, height, scale);
  if (!buffer.isNull())
    return buffer.save(fileName, format, quality);
  else
    return false;
}

QPixmap QCustomPlot::toPixmap(int width, int height, double scale)
{

  int newWidth, newHeight;
  if (width == 0 || height == 0)
  {
    newWidth = this->width();
    newHeight = this->height();
  } else
  {
    newWidth = width;
    newHeight = height;
  }
  int scaledWidth = qRound(scale*newWidth);
  int scaledHeight = qRound(scale*newHeight);

  QPixmap result(scaledWidth, scaledHeight);
  result.fill(mBackgroundBrush.style() == Qt::SolidPattern ? mBackgroundBrush.color() : Qt::transparent);
  QCPPainter painter;
  painter.begin(&result);
  if (painter.isActive())
  {
    QRect oldViewport = viewport();
    setViewport(QRect(0, 0, newWidth, newHeight));
    painter.setMode(QCPPainter::pmNoCaching);
    if (!qFuzzyCompare(scale, 1.0))
    {
      if (scale > 1.0)
        painter.setMode(QCPPainter::pmNonCosmetic);
      painter.scale(scale, scale);
    }
    if (mBackgroundBrush.style() != Qt::SolidPattern && mBackgroundBrush.style() != Qt::NoBrush)
      painter.fillRect(mViewport, mBackgroundBrush);
    draw(&painter);
    setViewport(oldViewport);
    painter.end();
  } else
  {
    qDebug() << Q_FUNC_INFO << "Couldn't activate painter on pixmap";
    return QPixmap();
  }
  return result;
}

void QCustomPlot::toPainter(QCPPainter *painter, int width, int height)
{

  int newWidth, newHeight;
  if (width == 0 || height == 0)
  {
    newWidth = this->width();
    newHeight = this->height();
  } else
  {
    newWidth = width;
    newHeight = height;
  }

  if (painter->isActive())
  {
    QRect oldViewport = viewport();
    setViewport(QRect(0, 0, newWidth, newHeight));
    painter->setMode(QCPPainter::pmNoCaching);

    if (mBackgroundBrush.style() != Qt::NoBrush)
      painter->fillRect(mViewport, mBackgroundBrush);
    draw(painter);
    setViewport(oldViewport);
  } else
    qDebug() << Q_FUNC_INFO << "Passed painter is not active";
}

QCPColorGradient::QCPColorGradient(GradientPreset preset) :
  mLevelCount(350),
  mColorInterpolation(ciRGB),
  mPeriodic(false),
  mColorBufferInvalidated(true)
{
  mColorBuffer.fill(qRgb(0, 0, 0), mLevelCount);
  loadPreset(preset);
}

bool QCPColorGradient::operator==(const QCPColorGradient &other) const
{
  return ((other.mLevelCount == this->mLevelCount) &&
          (other.mColorInterpolation == this->mColorInterpolation) &&
          (other.mPeriodic == this->mPeriodic) &&
          (other.mColorStops == this->mColorStops));
}

void QCPColorGradient::setLevelCount(int n)
{
  if (n < 2)
  {
    qDebug() << Q_FUNC_INFO << "n must be greater or equal 2 but was" << n;
    n = 2;
  }
  if (n != mLevelCount)
  {
    mLevelCount = n;
    mColorBufferInvalidated = true;
  }
}

void QCPColorGradient::setColorStops(const QMap<double, QColor> &colorStops)
{
  mColorStops = colorStops;
  mColorBufferInvalidated = true;
}

void QCPColorGradient::setColorStopAt(double position, const QColor &color)
{
  mColorStops.insert(position, color);
  mColorBufferInvalidated = true;
}

void QCPColorGradient::setColorInterpolation(QCPColorGradient::ColorInterpolation interpolation)
{
  if (interpolation != mColorInterpolation)
  {
    mColorInterpolation = interpolation;
    mColorBufferInvalidated = true;
  }
}

void QCPColorGradient::setPeriodic(bool enabled)
{
  mPeriodic = enabled;
}

void QCPColorGradient::colorize(const double *data, const QCPRange &range, QRgb *scanLine, int n, int dataIndexFactor, bool logarithmic)
{

  if (!data)
  {
    qDebug() << Q_FUNC_INFO << "null pointer given as data";
    return;
  }
  if (!scanLine)
  {
    qDebug() << Q_FUNC_INFO << "null pointer given as scanLine";
    return;
  }
  if (mColorBufferInvalidated)
    updateColorBuffer();

  if (!logarithmic)
  {
    const double posToIndexFactor = mLevelCount/range.size();
    if (mPeriodic)
    {
      for (int i=0; i<n; ++i)
      {
        int index = (int)((data[dataIndexFactor*i]-range.lower)*posToIndexFactor) % mLevelCount;
        if (index < 0)
          index += mLevelCount;
        scanLine[i] = mColorBuffer.at(index);
      }
    } else
    {
      for (int i=0; i<n; ++i)
      {
        int index = (data[dataIndexFactor*i]-range.lower)*posToIndexFactor;
        if (index < 0)
          index = 0;
        else if (index >= mLevelCount)
          index = mLevelCount-1;
        scanLine[i] = mColorBuffer.at(index);
      }
    }
  } else
  {
    if (mPeriodic)
    {
      for (int i=0; i<n; ++i)
      {
        int index = (int)(qLn(data[dataIndexFactor*i]/range.lower)/qLn(range.upper/range.lower)*mLevelCount) % mLevelCount;
        if (index < 0)
          index += mLevelCount;
        scanLine[i] = mColorBuffer.at(index);
      }
    } else
    {
      for (int i=0; i<n; ++i)
      {
        int index = qLn(data[dataIndexFactor*i]/range.lower)/qLn(range.upper/range.lower)*mLevelCount;
        if (index < 0)
          index = 0;
        else if (index >= mLevelCount)
          index = mLevelCount-1;
        scanLine[i] = mColorBuffer.at(index);
      }
    }
  }
}

QRgb QCPColorGradient::color(double position, const QCPRange &range, bool logarithmic)
{

  if (mColorBufferInvalidated)
    updateColorBuffer();
  int index = 0;
  if (!logarithmic)
    index = (position-range.lower)*mLevelCount/range.size();
  else
    index = qLn(position/range.lower)/qLn(range.upper/range.lower)*mLevelCount;
  if (mPeriodic)
  {
    index = index % mLevelCount;
    if (index < 0)
      index += mLevelCount;
  } else
  {
    if (index < 0)
      index = 0;
    else if (index >= mLevelCount)
      index = mLevelCount-1;
  }
  return mColorBuffer.at(index);
}

void QCPColorGradient::loadPreset(GradientPreset preset)
{
  clearColorStops();
  switch (preset)
  {
    case gpGrayscale:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, Qt::black);
      setColorStopAt(1, Qt::white);
      break;
    case gpHot:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(50, 0, 0));
      setColorStopAt(0.2, QColor(180, 10, 0));
      setColorStopAt(0.4, QColor(245, 50, 0));
      setColorStopAt(0.6, QColor(255, 150, 10));
      setColorStopAt(0.8, QColor(255, 255, 50));
      setColorStopAt(1, QColor(255, 255, 255));
      break;
    case gpCold:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(0, 0, 50));
      setColorStopAt(0.2, QColor(0, 10, 180));
      setColorStopAt(0.4, QColor(0, 50, 245));
      setColorStopAt(0.6, QColor(10, 150, 255));
      setColorStopAt(0.8, QColor(50, 255, 255));
      setColorStopAt(1, QColor(255, 255, 255));
      break;
    case gpNight:
      setColorInterpolation(ciHSV);
      setColorStopAt(0, QColor(10, 20, 30));
      setColorStopAt(1, QColor(250, 255, 250));
      break;
    case gpCandy:
      setColorInterpolation(ciHSV);
      setColorStopAt(0, QColor(0, 0, 255));
      setColorStopAt(1, QColor(255, 250, 250));
      break;
    case gpGeography:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(70, 170, 210));
      setColorStopAt(0.20, QColor(90, 160, 180));
      setColorStopAt(0.25, QColor(45, 130, 175));
      setColorStopAt(0.30, QColor(100, 140, 125));
      setColorStopAt(0.5, QColor(100, 140, 100));
      setColorStopAt(0.6, QColor(130, 145, 120));
      setColorStopAt(0.7, QColor(140, 130, 120));
      setColorStopAt(0.9, QColor(180, 190, 190));
      setColorStopAt(1, QColor(210, 210, 230));
      break;
    case gpIon:
      setColorInterpolation(ciHSV);
      setColorStopAt(0, QColor(50, 10, 10));
      setColorStopAt(0.45, QColor(0, 0, 255));
      setColorStopAt(0.8, QColor(0, 255, 255));
      setColorStopAt(1, QColor(0, 255, 0));
      break;
    case gpThermal:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(0, 0, 50));
      setColorStopAt(0.15, QColor(20, 0, 120));
      setColorStopAt(0.33, QColor(200, 30, 140));
      setColorStopAt(0.6, QColor(255, 100, 0));
      setColorStopAt(0.85, QColor(255, 255, 40));
      setColorStopAt(1, QColor(255, 255, 255));
      break;
    case gpPolar:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(50, 255, 255));
      setColorStopAt(0.18, QColor(10, 70, 255));
      setColorStopAt(0.28, QColor(10, 10, 190));
      setColorStopAt(0.5, QColor(0, 0, 0));
      setColorStopAt(0.72, QColor(190, 10, 10));
      setColorStopAt(0.82, QColor(255, 70, 10));
      setColorStopAt(1, QColor(255, 255, 50));
      break;
    case gpSpectrum:
      setColorInterpolation(ciHSV);
      setColorStopAt(0, QColor(50, 0, 50));
      setColorStopAt(0.15, QColor(0, 0, 255));
      setColorStopAt(0.35, QColor(0, 255, 255));
      setColorStopAt(0.6, QColor(255, 255, 0));
      setColorStopAt(0.75, QColor(255, 30, 0));
      setColorStopAt(1, QColor(50, 0, 0));
      break;
    case gpJet:
      setColorInterpolation(ciRGB);
      setColorStopAt(0, QColor(0, 0, 100));
      setColorStopAt(0.15, QColor(0, 50, 255));
      setColorStopAt(0.35, QColor(0, 255, 255));
      setColorStopAt(0.65, QColor(255, 255, 0));
      setColorStopAt(0.85, QColor(255, 30, 0));
      setColorStopAt(1, QColor(100, 0, 0));
      break;
    case gpHues:
      setColorInterpolation(ciHSV);
      setColorStopAt(0, QColor(255, 0, 0));
      setColorStopAt(1.0/3.0, QColor(0, 0, 255));
      setColorStopAt(2.0/3.0, QColor(0, 255, 0));
      setColorStopAt(1, QColor(255, 0, 0));
      break;
  }
}

void QCPColorGradient::clearColorStops()
{
  mColorStops.clear();
  mColorBufferInvalidated = true;
}

QCPColorGradient QCPColorGradient::inverted() const
{
  QCPColorGradient result(*this);
  result.clearColorStops();
  for (QMap<double, QColor>::const_iterator it=mColorStops.constBegin(); it!=mColorStops.constEnd(); ++it)
    result.setColorStopAt(1.0-it.key(), it.value());
  return result;
}

void QCPColorGradient::updateColorBuffer()
{
  if (mColorBuffer.size() != mLevelCount)
    mColorBuffer.resize(mLevelCount);
  if (mColorStops.size() > 1)
  {
    double indexToPosFactor = 1.0/(double)(mLevelCount-1);
    for (int i=0; i<mLevelCount; ++i)
    {
      double position = i*indexToPosFactor;
      QMap<double, QColor>::const_iterator it = mColorStops.lowerBound(position);
      if (it == mColorStops.constEnd())
      {
        mColorBuffer[i] = (it-1).value().rgb();
      } else if (it == mColorStops.constBegin())
      {
        mColorBuffer[i] = it.value().rgb();
      } else
      {
        QMap<double, QColor>::const_iterator high = it;
        QMap<double, QColor>::const_iterator low = it-1;
        double t = (position-low.key())/(high.key()-low.key());
        switch (mColorInterpolation)
        {
          case ciRGB:
          {
            mColorBuffer[i] = qRgb((1-t)*low.value().red() + t*high.value().red(),
                                   (1-t)*low.value().green() + t*high.value().green(),
                                   (1-t)*low.value().blue() + t*high.value().blue());
            break;
          }
          case ciHSV:
          {
            QColor lowHsv = low.value().toHsv();
            QColor highHsv = high.value().toHsv();
            double hue = 0;
            double hueDiff = highHsv.hueF()-lowHsv.hueF();
            if (hueDiff > 0.5)
              hue = lowHsv.hueF() - t*(1.0-hueDiff);
            else if (hueDiff < -0.5)
              hue = lowHsv.hueF() + t*(1.0+hueDiff);
            else
              hue = lowHsv.hueF() + t*hueDiff;
            if (hue < 0) hue += 1.0;
            else if (hue >= 1.0) hue -= 1.0;
            mColorBuffer[i] = QColor::fromHsvF(hue, (1-t)*lowHsv.saturationF() + t*highHsv.saturationF(), (1-t)*lowHsv.valueF() + t*highHsv.valueF()).rgb();
            break;
          }
        }
      }
    }
  } else if (mColorStops.size() == 1)
  {
    mColorBuffer.fill(mColorStops.constBegin().value().rgb());
  } else
  {
    mColorBuffer.fill(qRgb(0, 0, 0));
  }
  mColorBufferInvalidated = false;
}

QCPAxisRect::QCPAxisRect(QCustomPlot *parentPlot, bool setupDefaultAxes) :
  QCPLayoutElement(parentPlot),
  mBackgroundBrush(Qt::NoBrush),
  mBackgroundScaled(true),
  mBackgroundScaledMode(Qt::KeepAspectRatioByExpanding),
  mInsetLayout(new QCPLayoutInset),
  mRangeDrag(Qt::Horizontal|Qt::Vertical),
  mRangeZoom(Qt::Horizontal|Qt::Vertical),
  mRangeZoomFactorHorz(0.85),
  mRangeZoomFactorVert(0.85),
  mDragging(false)
{
  mInsetLayout->initializeParentPlot(mParentPlot);
  mInsetLayout->setParentLayerable(this);
  mInsetLayout->setParent(this);

  setMinimumSize(50, 50);
  setMinimumMargins(QMargins(15, 15, 15, 15));
  mAxes.insert(QCPAxis::atLeft, QList<QCPAxis*>());
  mAxes.insert(QCPAxis::atRight, QList<QCPAxis*>());
  mAxes.insert(QCPAxis::atTop, QList<QCPAxis*>());
  mAxes.insert(QCPAxis::atBottom, QList<QCPAxis*>());

  if (setupDefaultAxes)
  {
    QCPAxis *xAxis = addAxis(QCPAxis::atBottom);
    QCPAxis *yAxis = addAxis(QCPAxis::atLeft);
    QCPAxis *xAxis2 = addAxis(QCPAxis::atTop);
    QCPAxis *yAxis2 = addAxis(QCPAxis::atRight);
    setRangeDragAxes(xAxis, yAxis);
    setRangeZoomAxes(xAxis, yAxis);
    xAxis2->setVisible(false);
    yAxis2->setVisible(false);
    xAxis->grid()->setVisible(true);
    yAxis->grid()->setVisible(true);
    xAxis2->grid()->setVisible(false);
    yAxis2->grid()->setVisible(false);
    xAxis2->grid()->setZeroLinePen(Qt::NoPen);
    yAxis2->grid()->setZeroLinePen(Qt::NoPen);
    xAxis2->grid()->setVisible(false);
    yAxis2->grid()->setVisible(false);
  }
}

QCPAxisRect::~QCPAxisRect()
{
  delete mInsetLayout;
  mInsetLayout = 0;

  QList<QCPAxis*> axesList = axes();
  for (int i=0; i<axesList.size(); ++i)
    removeAxis(axesList.at(i));
}

int QCPAxisRect::axisCount(QCPAxis::AxisType type) const
{
  return mAxes.value(type).size();
}

QCPAxis *QCPAxisRect::axis(QCPAxis::AxisType type, int index) const
{
  QList<QCPAxis*> ax(mAxes.value(type));
  if (index >= 0 && index < ax.size())
  {
    return ax.at(index);
  } else
  {
    qDebug() << Q_FUNC_INFO << "Axis index out of bounds:" << index;
    return 0;
  }
}

QList<QCPAxis*> QCPAxisRect::axes(QCPAxis::AxisTypes types) const
{
  QList<QCPAxis*> result;
  if (types.testFlag(QCPAxis::atLeft))
    result << mAxes.value(QCPAxis::atLeft);
  if (types.testFlag(QCPAxis::atRight))
    result << mAxes.value(QCPAxis::atRight);
  if (types.testFlag(QCPAxis::atTop))
    result << mAxes.value(QCPAxis::atTop);
  if (types.testFlag(QCPAxis::atBottom))
    result << mAxes.value(QCPAxis::atBottom);
  return result;
}

QList<QCPAxis*> QCPAxisRect::axes() const
{
  QList<QCPAxis*> result;
  QHashIterator<QCPAxis::AxisType, QList<QCPAxis*> > it(mAxes);
  while (it.hasNext())
  {
    it.next();
    result << it.value();
  }
  return result;
}

QCPAxis *QCPAxisRect::addAxis(QCPAxis::AxisType type)
{
  QCPAxis *newAxis = new QCPAxis(this, type);
  if (mAxes[type].size() > 0)
  {
    bool invert = (type == QCPAxis::atRight) || (type == QCPAxis::atBottom);
    newAxis->setLowerEnding(QCPLineEnding(QCPLineEnding::esHalfBar, 6, 10, !invert));
    newAxis->setUpperEnding(QCPLineEnding(QCPLineEnding::esHalfBar, 6, 10, invert));
  }
  mAxes[type].append(newAxis);
  return newAxis;
}

QList<QCPAxis*> QCPAxisRect::addAxes(QCPAxis::AxisTypes types)
{
  QList<QCPAxis*> result;
  if (types.testFlag(QCPAxis::atLeft))
    result << addAxis(QCPAxis::atLeft);
  if (types.testFlag(QCPAxis::atRight))
    result << addAxis(QCPAxis::atRight);
  if (types.testFlag(QCPAxis::atTop))
    result << addAxis(QCPAxis::atTop);
  if (types.testFlag(QCPAxis::atBottom))
    result << addAxis(QCPAxis::atBottom);
  return result;
}

bool QCPAxisRect::removeAxis(QCPAxis *axis)
{

  QHashIterator<QCPAxis::AxisType, QList<QCPAxis*> > it(mAxes);
  while (it.hasNext())
  {
    it.next();
    if (it.value().contains(axis))
    {
      mAxes[it.key()].removeOne(axis);
      if (qobject_cast<QCustomPlot*>(parentPlot()))
        parentPlot()->axisRemoved(axis);
      delete axis;
      return true;
    }
  }
  qDebug() << Q_FUNC_INFO << "Axis isn't in axis rect:" << reinterpret_cast<quintptr>(axis);
  return false;
}

void QCPAxisRect::setupFullAxesBox(bool connectRanges)
{
  QCPAxis *xAxis, *yAxis, *xAxis2, *yAxis2;
  if (axisCount(QCPAxis::atBottom) == 0)
    xAxis = addAxis(QCPAxis::atBottom);
  else
    xAxis = axis(QCPAxis::atBottom);

  if (axisCount(QCPAxis::atLeft) == 0)
    yAxis = addAxis(QCPAxis::atLeft);
  else
    yAxis = axis(QCPAxis::atLeft);

  if (axisCount(QCPAxis::atTop) == 0)
    xAxis2 = addAxis(QCPAxis::atTop);
  else
    xAxis2 = axis(QCPAxis::atTop);

  if (axisCount(QCPAxis::atRight) == 0)
    yAxis2 = addAxis(QCPAxis::atRight);
  else
    yAxis2 = axis(QCPAxis::atRight);

  xAxis->setVisible(true);
  yAxis->setVisible(true);
  xAxis2->setVisible(true);
  yAxis2->setVisible(true);
  xAxis2->setTickLabels(false);
  yAxis2->setTickLabels(false);

  xAxis2->setRange(xAxis->range());
  xAxis2->setRangeReversed(xAxis->rangeReversed());
  xAxis2->setScaleType(xAxis->scaleType());
  xAxis2->setScaleLogBase(xAxis->scaleLogBase());
  xAxis2->setTicks(xAxis->ticks());
  xAxis2->setAutoTickCount(xAxis->autoTickCount());
  xAxis2->setSubTickCount(xAxis->subTickCount());
  xAxis2->setAutoSubTicks(xAxis->autoSubTicks());
  xAxis2->setTickStep(xAxis->tickStep());
  xAxis2->setAutoTickStep(xAxis->autoTickStep());
  xAxis2->setNumberFormat(xAxis->numberFormat());
  xAxis2->setNumberPrecision(xAxis->numberPrecision());
  xAxis2->setTickLabelType(xAxis->tickLabelType());
  xAxis2->setDateTimeFormat(xAxis->dateTimeFormat());
  xAxis2->setDateTimeSpec(xAxis->dateTimeSpec());

  yAxis2->setRange(yAxis->range());
  yAxis2->setRangeReversed(yAxis->rangeReversed());
  yAxis2->setScaleType(yAxis->scaleType());
  yAxis2->setScaleLogBase(yAxis->scaleLogBase());
  yAxis2->setTicks(yAxis->ticks());
  yAxis2->setAutoTickCount(yAxis->autoTickCount());
  yAxis2->setSubTickCount(yAxis->subTickCount());
  yAxis2->setAutoSubTicks(yAxis->autoSubTicks());
  yAxis2->setTickStep(yAxis->tickStep());
  yAxis2->setAutoTickStep(yAxis->autoTickStep());
  yAxis2->setNumberFormat(yAxis->numberFormat());
  yAxis2->setNumberPrecision(yAxis->numberPrecision());
  yAxis2->setTickLabelType(yAxis->tickLabelType());
  yAxis2->setDateTimeFormat(yAxis->dateTimeFormat());
  yAxis2->setDateTimeSpec(yAxis->dateTimeSpec());

  if (connectRanges)
  {
    connect(xAxis, SIGNAL(rangeChanged(QCPRange)), xAxis2, SLOT(setRange(QCPRange)));
    connect(yAxis, SIGNAL(rangeChanged(QCPRange)), yAxis2, SLOT(setRange(QCPRange)));
  }
}

QList<QCPAbstractPlottable*> QCPAxisRect::plottables() const
{

  QList<QCPAbstractPlottable*> result;
  for (int i=0; i<mParentPlot->mPlottables.size(); ++i)
  {
    if (mParentPlot->mPlottables.at(i)->keyAxis()->axisRect() == this ||mParentPlot->mPlottables.at(i)->valueAxis()->axisRect() == this)
      result.append(mParentPlot->mPlottables.at(i));
  }
  return result;
}

QList<QCPGraph*> QCPAxisRect::graphs() const
{

  QList<QCPGraph*> result;
  for (int i=0; i<mParentPlot->mGraphs.size(); ++i)
  {
    if (mParentPlot->mGraphs.at(i)->keyAxis()->axisRect() == this || mParentPlot->mGraphs.at(i)->valueAxis()->axisRect() == this)
      result.append(mParentPlot->mGraphs.at(i));
  }
  return result;
}

QList<QCPAbstractItem *> QCPAxisRect::items() const
{

  QList<QCPAbstractItem*> result;
  for (int itemId=0; itemId<mParentPlot->mItems.size(); ++itemId)
  {
    if (mParentPlot->mItems.at(itemId)->clipAxisRect() == this)
    {
      result.append(mParentPlot->mItems.at(itemId));
      continue;
    }
    QList<QCPItemPosition*> positions = mParentPlot->mItems.at(itemId)->positions();
    for (int posId=0; posId<positions.size(); ++posId)
    {
      if (positions.at(posId)->axisRect() == this ||
          positions.at(posId)->keyAxis()->axisRect() == this ||
          positions.at(posId)->valueAxis()->axisRect() == this)
      {
        result.append(mParentPlot->mItems.at(itemId));
        break;
      }
    }
  }
  return result;
}

void QCPAxisRect::update(UpdatePhase phase)
{
  QCPLayoutElement::update(phase);

  switch (phase)
  {
    case upPreparation:
    {
      QList<QCPAxis*> allAxes = axes();
      for (int i=0; i<allAxes.size(); ++i)
        allAxes.at(i)->setupTickVectors();
      break;
    }
    case upLayout:
    {
      mInsetLayout->setOuterRect(rect());
      break;
    }
    default: break;
  }

  mInsetLayout->update(phase);
}

QList<QCPLayoutElement*> QCPAxisRect::elements(bool recursive) const
{
  QList<QCPLayoutElement*> result;
  if (mInsetLayout)
  {
    result << mInsetLayout;
    if (recursive)
      result << mInsetLayout->elements(recursive);
  }
  return result;
}

void QCPAxisRect::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  painter->setAntialiasing(false);
}

void QCPAxisRect::draw(QCPPainter *painter)
{
  drawBackground(painter);
}

void QCPAxisRect::setBackground(const QPixmap &pm)
{
  mBackgroundPixmap = pm;
  mScaledBackgroundPixmap = QPixmap();
}

void QCPAxisRect::setBackground(const QBrush &brush)
{
  mBackgroundBrush = brush;
}

void QCPAxisRect::setBackground(const QPixmap &pm, bool scaled, Qt::AspectRatioMode mode)
{
  mBackgroundPixmap = pm;
  mScaledBackgroundPixmap = QPixmap();
  mBackgroundScaled = scaled;
  mBackgroundScaledMode = mode;
}

void QCPAxisRect::setBackgroundScaled(bool scaled)
{
  mBackgroundScaled = scaled;
}

void QCPAxisRect::setBackgroundScaledMode(Qt::AspectRatioMode mode)
{
  mBackgroundScaledMode = mode;
}

QCPAxis *QCPAxisRect::rangeDragAxis(Qt::Orientation orientation)
{
  return (orientation == Qt::Horizontal ? mRangeDragHorzAxis.data() : mRangeDragVertAxis.data());
}

QCPAxis *QCPAxisRect::rangeZoomAxis(Qt::Orientation orientation)
{
  return (orientation == Qt::Horizontal ? mRangeZoomHorzAxis.data() : mRangeZoomVertAxis.data());
}

double QCPAxisRect::rangeZoomFactor(Qt::Orientation orientation)
{
  return (orientation == Qt::Horizontal ? mRangeZoomFactorHorz : mRangeZoomFactorVert);
}

void QCPAxisRect::setRangeDrag(Qt::Orientations orientations)
{
  mRangeDrag = orientations;
}

void QCPAxisRect::setRangeZoom(Qt::Orientations orientations)
{
  mRangeZoom = orientations;
}

void QCPAxisRect::setRangeDragAxes(QCPAxis *horizontal, QCPAxis *vertical)
{
  mRangeDragHorzAxis = horizontal;
  mRangeDragVertAxis = vertical;
}

void QCPAxisRect::setRangeZoomAxes(QCPAxis *horizontal, QCPAxis *vertical)
{
  mRangeZoomHorzAxis = horizontal;
  mRangeZoomVertAxis = vertical;
}

void QCPAxisRect::setRangeZoomFactor(double horizontalFactor, double verticalFactor)
{
  mRangeZoomFactorHorz = horizontalFactor;
  mRangeZoomFactorVert = verticalFactor;
}

void QCPAxisRect::setRangeZoomFactor(double factor)
{
  mRangeZoomFactorHorz = factor;
  mRangeZoomFactorVert = factor;
}

void QCPAxisRect::drawBackground(QCPPainter *painter)
{

  if (mBackgroundBrush != Qt::NoBrush)
    painter->fillRect(mRect, mBackgroundBrush);

  if (!mBackgroundPixmap.isNull())
  {
    if (mBackgroundScaled)
    {

      QSize scaledSize(mBackgroundPixmap.size());
      scaledSize.scale(mRect.size(), mBackgroundScaledMode);
      if (mScaledBackgroundPixmap.size() != scaledSize)
        mScaledBackgroundPixmap = mBackgroundPixmap.scaled(mRect.size(), mBackgroundScaledMode, Qt::SmoothTransformation);
      painter->drawPixmap(mRect.topLeft(), mScaledBackgroundPixmap, QRect(0, 0, mRect.width(), mRect.height()) & mScaledBackgroundPixmap.rect());
    } else
    {
      painter->drawPixmap(mRect.topLeft(), mBackgroundPixmap, QRect(0, 0, mRect.width(), mRect.height()));
    }
  }
}

void QCPAxisRect::updateAxesOffset(QCPAxis::AxisType type)
{
  const QList<QCPAxis*> axesList = mAxes.value(type);
  if (axesList.isEmpty())
    return;

  bool isFirstVisible = !axesList.first()->visible();
  for (int i=1; i<axesList.size(); ++i)
  {
    int offset = axesList.at(i-1)->offset() + axesList.at(i-1)->calculateMargin();
    if (axesList.at(i)->visible())
    {
      if (!isFirstVisible)
        offset += axesList.at(i)->tickLengthIn();
      isFirstVisible = false;
    }
    axesList.at(i)->setOffset(offset);
  }
}

int QCPAxisRect::calculateAutoMargin(QCP::MarginSide side)
{
  if (!mAutoMargins.testFlag(side))
    qDebug() << Q_FUNC_INFO << "Called with side that isn't specified as auto margin";

  updateAxesOffset(QCPAxis::marginSideToAxisType(side));

  const QList<QCPAxis*> axesList = mAxes.value(QCPAxis::marginSideToAxisType(side));
  if (axesList.size() > 0)
    return axesList.last()->offset() + axesList.last()->calculateMargin();
  else
    return 0;
}

void QCPAxisRect::mousePressEvent(QMouseEvent *event)
{
  mDragStart = event->pos();
  if (event->buttons() & Qt::LeftButton)
  {
    mDragging = true;

    if (mParentPlot->noAntialiasingOnDrag())
    {
      mAADragBackup = mParentPlot->antialiasedElements();
      mNotAADragBackup = mParentPlot->notAntialiasedElements();
    }

    if (mParentPlot->interactions().testFlag(QCP::iRangeDrag))
    {
      if (mRangeDragHorzAxis)
        mDragStartHorzRange = mRangeDragHorzAxis.data()->range();
      if (mRangeDragVertAxis)
        mDragStartVertRange = mRangeDragVertAxis.data()->range();
    }
  }
}

void QCPAxisRect::mouseMoveEvent(QMouseEvent *event)
{

  if (mDragging && mParentPlot->interactions().testFlag(QCP::iRangeDrag))
  {
    if (mRangeDrag.testFlag(Qt::Horizontal))
    {
      if (QCPAxis *rangeDragHorzAxis = mRangeDragHorzAxis.data())
      {
        if (rangeDragHorzAxis->mScaleType == QCPAxis::stLinear)
        {
          double diff = rangeDragHorzAxis->pixelToCoord(mDragStart.x()) - rangeDragHorzAxis->pixelToCoord(event->pos().x());
          rangeDragHorzAxis->setRange(mDragStartHorzRange.lower+diff, mDragStartHorzRange.upper+diff);
        } else if (rangeDragHorzAxis->mScaleType == QCPAxis::stLogarithmic)
        {
          double diff = rangeDragHorzAxis->pixelToCoord(mDragStart.x()) / rangeDragHorzAxis->pixelToCoord(event->pos().x());
          rangeDragHorzAxis->setRange(mDragStartHorzRange.lower*diff, mDragStartHorzRange.upper*diff);
        }
      }
    }
    if (mRangeDrag.testFlag(Qt::Vertical))
    {
      if (QCPAxis *rangeDragVertAxis = mRangeDragVertAxis.data())
      {
        if (rangeDragVertAxis->mScaleType == QCPAxis::stLinear)
        {
          double diff = rangeDragVertAxis->pixelToCoord(mDragStart.y()) - rangeDragVertAxis->pixelToCoord(event->pos().y());
          rangeDragVertAxis->setRange(mDragStartVertRange.lower+diff, mDragStartVertRange.upper+diff);
        } else if (rangeDragVertAxis->mScaleType == QCPAxis::stLogarithmic)
        {
          double diff = rangeDragVertAxis->pixelToCoord(mDragStart.y()) / rangeDragVertAxis->pixelToCoord(event->pos().y());
          rangeDragVertAxis->setRange(mDragStartVertRange.lower*diff, mDragStartVertRange.upper*diff);
        }
      }
    }
    if (mRangeDrag != 0)
    {
      if (mParentPlot->noAntialiasingOnDrag())
        mParentPlot->setNotAntialiasedElements(QCP::aeAll);
      mParentPlot->replot();
    }
  }
}

void QCPAxisRect::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  mDragging = false;
  if (mParentPlot->noAntialiasingOnDrag())
  {
    mParentPlot->setAntialiasedElements(mAADragBackup);
    mParentPlot->setNotAntialiasedElements(mNotAADragBackup);
  }
}

void QCPAxisRect::wheelEvent(QWheelEvent *event)
{

  if (mParentPlot->interactions().testFlag(QCP::iRangeZoom))
  {
    if (mRangeZoom != 0)
    {
      double factor;
      double wheelSteps = event->delta()/120.0;
      if (mRangeZoom.testFlag(Qt::Horizontal))
      {
        factor = pow(mRangeZoomFactorHorz, wheelSteps);
        if (mRangeZoomHorzAxis.data())
          mRangeZoomHorzAxis.data()->scaleRange(factor, mRangeZoomHorzAxis.data()->pixelToCoord(event->pos().x()));
      }
      if (mRangeZoom.testFlag(Qt::Vertical))
      {
        factor = pow(mRangeZoomFactorVert, wheelSteps);
        if (mRangeZoomVertAxis.data())
          mRangeZoomVertAxis.data()->scaleRange(factor, mRangeZoomVertAxis.data()->pixelToCoord(event->pos().y()));
      }
      mParentPlot->replot();
    }
  }
}

QCPAbstractLegendItem::QCPAbstractLegendItem(QCPLegend *parent) :
  QCPLayoutElement(parent->parentPlot()),
  mParentLegend(parent),
  mFont(parent->font()),
  mTextColor(parent->textColor()),
  mSelectedFont(parent->selectedFont()),
  mSelectedTextColor(parent->selectedTextColor()),
  mSelectable(true),
  mSelected(false)
{
  setLayer("legend");
  setMargins(QMargins(8, 2, 8, 2));
}

void QCPAbstractLegendItem::setFont(const QFont &font)
{
  mFont = font;
}

void QCPAbstractLegendItem::setTextColor(const QColor &color)
{
  mTextColor = color;
}

void QCPAbstractLegendItem::setSelectedFont(const QFont &font)
{
  mSelectedFont = font;
}

void QCPAbstractLegendItem::setSelectedTextColor(const QColor &color)
{
  mSelectedTextColor = color;
}

void QCPAbstractLegendItem::setSelectable(bool selectable)
{
  if (mSelectable != selectable)
  {
    mSelectable = selectable;
    Q_EMIT selectableChanged(mSelectable);
  }
}

void QCPAbstractLegendItem::setSelected(bool selected)
{
  if (mSelected != selected)
  {
    mSelected = selected;
    Q_EMIT selectionChanged(mSelected);
  }
}

double QCPAbstractLegendItem::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (!mParentPlot) return -1;
  if (onlySelectable && (!mSelectable || !mParentLegend->selectableParts().testFlag(QCPLegend::spItems)))
    return -1;

  if (mRect.contains(pos.toPoint()))
    return mParentPlot->selectionTolerance()*0.99;
  else
    return -1;
}

void QCPAbstractLegendItem::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeLegendItems);
}

QRect QCPAbstractLegendItem::clipRect() const
{
  return mOuterRect;
}

void QCPAbstractLegendItem::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  Q_UNUSED(details)
  if (mSelectable && mParentLegend->selectableParts().testFlag(QCPLegend::spItems))
  {
    bool selBefore = mSelected;
    setSelected(additive ? !mSelected : true);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

void QCPAbstractLegendItem::deselectEvent(bool *selectionStateChanged)
{
  if (mSelectable && mParentLegend->selectableParts().testFlag(QCPLegend::spItems))
  {
    bool selBefore = mSelected;
    setSelected(false);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

QCPPlottableLegendItem::QCPPlottableLegendItem(QCPLegend *parent, QCPAbstractPlottable *plottable) :
  QCPAbstractLegendItem(parent),
  mPlottable(plottable)
{
}

QPen QCPPlottableLegendItem::getIconBorderPen() const
{
  return mSelected ? mParentLegend->selectedIconBorderPen() : mParentLegend->iconBorderPen();
}

QColor QCPPlottableLegendItem::getTextColor() const
{
  return mSelected ? mSelectedTextColor : mTextColor;
}

QFont QCPPlottableLegendItem::getFont() const
{
  return mSelected ? mSelectedFont : mFont;
}

void QCPPlottableLegendItem::draw(QCPPainter *painter)
{
  if (!mPlottable) return;
  painter->setFont(getFont());
  painter->setPen(QPen(getTextColor()));
  QSizeF iconSize = mParentLegend->iconSize();
  QRectF textRect = painter->fontMetrics().boundingRect(0, 0, 0, iconSize.height(), Qt::TextDontClip, mPlottable->name());
  QRectF iconRect(mRect.topLeft(), iconSize);
  int textHeight = qMax(textRect.height(), iconSize.height());
  painter->drawText(mRect.x()+iconSize.width()+mParentLegend->iconTextPadding(), mRect.y(), textRect.width(), textHeight, Qt::TextDontClip, mPlottable->name());

  painter->save();
  painter->setClipRect(iconRect, Qt::IntersectClip);
  mPlottable->drawLegendIcon(painter, iconRect);
  painter->restore();

  if (getIconBorderPen().style() != Qt::NoPen)
  {
    painter->setPen(getIconBorderPen());
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(iconRect);
  }
}

QSize QCPPlottableLegendItem::minimumSizeHint() const
{
  if (!mPlottable) return QSize();
  QSize result(0, 0);
  QRect textRect;
  QFontMetrics fontMetrics(getFont());
  QSize iconSize = mParentLegend->iconSize();
  textRect = fontMetrics.boundingRect(0, 0, 0, iconSize.height(), Qt::TextDontClip, mPlottable->name());
  result.setWidth(iconSize.width() + mParentLegend->iconTextPadding() + textRect.width() + mMargins.left() + mMargins.right());
  result.setHeight(qMax(textRect.height(), iconSize.height()) + mMargins.top() + mMargins.bottom());
  return result;
}

QCPLegend::QCPLegend()
{
  setRowSpacing(0);
  setColumnSpacing(10);
  setMargins(QMargins(2, 3, 2, 2));
  setAntialiased(false);
  setIconSize(32, 18);

  setIconTextPadding(7);

  setSelectableParts(spLegendBox | spItems);
  setSelectedParts(spNone);

  setBorderPen(QPen(Qt::black));
  setSelectedBorderPen(QPen(Qt::blue, 2));
  setIconBorderPen(Qt::NoPen);
  setSelectedIconBorderPen(QPen(Qt::blue, 2));
  setBrush(Qt::white);
  setSelectedBrush(Qt::white);
  setTextColor(Qt::black);
  setSelectedTextColor(Qt::blue);
}

QCPLegend::~QCPLegend()
{
  clearItems();
  if (mParentPlot)
    mParentPlot->legendRemoved(this);
}

QCPLegend::SelectableParts QCPLegend::selectedParts() const
{

  bool hasSelectedItems = false;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i) && item(i)->selected())
    {
      hasSelectedItems = true;
      break;
    }
  }
  if (hasSelectedItems)
    return mSelectedParts | spItems;
  else
    return mSelectedParts & ~spItems;
}

void QCPLegend::setBorderPen(const QPen &pen)
{
  mBorderPen = pen;
}

void QCPLegend::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPLegend::setFont(const QFont &font)
{
  mFont = font;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setFont(mFont);
  }
}

void QCPLegend::setTextColor(const QColor &color)
{
  mTextColor = color;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setTextColor(color);
  }
}

void QCPLegend::setIconSize(const QSize &size)
{
  mIconSize = size;
}

void QCPLegend::setIconSize(int width, int height)
{
  mIconSize.setWidth(width);
  mIconSize.setHeight(height);
}

void QCPLegend::setIconTextPadding(int padding)
{
  mIconTextPadding = padding;
}

void QCPLegend::setIconBorderPen(const QPen &pen)
{
  mIconBorderPen = pen;
}

void QCPLegend::setSelectableParts(const SelectableParts &selectable)
{
  if (mSelectableParts != selectable)
  {
    mSelectableParts = selectable;
    Q_EMIT selectableChanged(mSelectableParts);
  }
}

void QCPLegend::setSelectedParts(const SelectableParts &selected)
{
  SelectableParts newSelected = selected;
  mSelectedParts = this->selectedParts();

  if (mSelectedParts != newSelected)
  {
    if (!mSelectedParts.testFlag(spItems) && newSelected.testFlag(spItems))
    {
      qDebug() << Q_FUNC_INFO << "spItems flag can not be set, it can only be unset with this function";
      newSelected &= ~spItems;
    }
    if (mSelectedParts.testFlag(spItems) && !newSelected.testFlag(spItems))
    {
      for (int i=0; i<itemCount(); ++i)
      {
        if (item(i))
          item(i)->setSelected(false);
      }
    }
    mSelectedParts = newSelected;
    Q_EMIT selectionChanged(mSelectedParts);
  }
}

void QCPLegend::setSelectedBorderPen(const QPen &pen)
{
  mSelectedBorderPen = pen;
}

void QCPLegend::setSelectedIconBorderPen(const QPen &pen)
{
  mSelectedIconBorderPen = pen;
}

void QCPLegend::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

void QCPLegend::setSelectedFont(const QFont &font)
{
  mSelectedFont = font;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setSelectedFont(font);
  }
}

void QCPLegend::setSelectedTextColor(const QColor &color)
{
  mSelectedTextColor = color;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setSelectedTextColor(color);
  }
}

QCPAbstractLegendItem *QCPLegend::item(int index) const
{
  return qobject_cast<QCPAbstractLegendItem*>(elementAt(index));
}

QCPPlottableLegendItem *QCPLegend::itemWithPlottable(const QCPAbstractPlottable *plottable) const
{
  for (int i=0; i<itemCount(); ++i)
  {
    if (QCPPlottableLegendItem *pli = qobject_cast<QCPPlottableLegendItem*>(item(i)))
    {
      if (pli->plottable() == plottable)
        return pli;
    }
  }
  return 0;
}

int QCPLegend::itemCount() const
{
  return elementCount();
}

bool QCPLegend::hasItem(QCPAbstractLegendItem *item) const
{
  for (int i=0; i<itemCount(); ++i)
  {
    if (item == this->item(i))
        return true;
  }
  return false;
}

bool QCPLegend::hasItemWithPlottable(const QCPAbstractPlottable *plottable) const
{
  return itemWithPlottable(plottable);
}

bool QCPLegend::addItem(QCPAbstractLegendItem *item)
{
  if (!hasItem(item))
  {
    return addElement(rowCount(), 0, item);
  } else
    return false;
}

bool QCPLegend::removeItem(int index)
{
  if (QCPAbstractLegendItem *ali = item(index))
  {
    bool success = remove(ali);
    simplify();
    return success;
  } else
    return false;
}

bool QCPLegend::removeItem(QCPAbstractLegendItem *item)
{
  bool success = remove(item);
  simplify();
  return success;
}

void QCPLegend::clearItems()
{
  for (int i=itemCount()-1; i>=0; --i)
    removeItem(i);
}

QList<QCPAbstractLegendItem *> QCPLegend::selectedItems() const
{
  QList<QCPAbstractLegendItem*> result;
  for (int i=0; i<itemCount(); ++i)
  {
    if (QCPAbstractLegendItem *ali = item(i))
    {
      if (ali->selected())
        result.append(ali);
    }
  }
  return result;
}

void QCPLegend::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeLegend);
}

QPen QCPLegend::getBorderPen() const
{
  return mSelectedParts.testFlag(spLegendBox) ? mSelectedBorderPen : mBorderPen;
}

QBrush QCPLegend::getBrush() const
{
  return mSelectedParts.testFlag(spLegendBox) ? mSelectedBrush : mBrush;
}

void QCPLegend::draw(QCPPainter *painter)
{

  painter->setBrush(getBrush());
  painter->setPen(getBorderPen());
  painter->drawRect(mOuterRect);
}

double QCPLegend::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  if (!mParentPlot) return -1;
  if (onlySelectable && !mSelectableParts.testFlag(spLegendBox))
    return -1;

  if (mOuterRect.contains(pos.toPoint()))
  {
    if (details) details->setValue(spLegendBox);
    return mParentPlot->selectionTolerance()*0.99;
  }
  return -1;
}

void QCPLegend::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  mSelectedParts = selectedParts();
  if (details.value<SelectablePart>() == spLegendBox && mSelectableParts.testFlag(spLegendBox))
  {
    SelectableParts selBefore = mSelectedParts;
    setSelectedParts(additive ? mSelectedParts^spLegendBox : mSelectedParts|spLegendBox);
    if (selectionStateChanged)
      *selectionStateChanged = mSelectedParts != selBefore;
  }
}

void QCPLegend::deselectEvent(bool *selectionStateChanged)
{
  mSelectedParts = selectedParts();
  if (mSelectableParts.testFlag(spLegendBox))
  {
    SelectableParts selBefore = mSelectedParts;
    setSelectedParts(selectedParts() & ~spLegendBox);
    if (selectionStateChanged)
      *selectionStateChanged = mSelectedParts != selBefore;
  }
}

QCP::Interaction QCPLegend::selectionCategory() const
{
  return QCP::iSelectLegend;
}

QCP::Interaction QCPAbstractLegendItem::selectionCategory() const
{
  return QCP::iSelectLegend;
}

void QCPLegend::parentPlotInitialized(QCustomPlot *parentPlot)
{
  Q_UNUSED(parentPlot)
}

QCPPlotTitle::QCPPlotTitle(QCustomPlot *parentPlot) :
  QCPLayoutElement(parentPlot),
  mFont(QFont("sans serif", 13*1.5, QFont::Bold)),
  mTextColor(Qt::black),
  mSelectedFont(QFont("sans serif", 13*1.6, QFont::Bold)),
  mSelectedTextColor(Qt::blue),
  mSelectable(false),
  mSelected(false)
{
  if (parentPlot)
  {
    setLayer(parentPlot->currentLayer());
    mFont = QFont(parentPlot->font().family(), parentPlot->font().pointSize()*1.5, QFont::Bold);
    mSelectedFont = QFont(parentPlot->font().family(), parentPlot->font().pointSize()*1.6, QFont::Bold);
  }
  setMargins(QMargins(5, 5, 5, 0));
}

QCPPlotTitle::QCPPlotTitle(QCustomPlot *parentPlot, const QString &text) :
  QCPLayoutElement(parentPlot),
  mText(text),
  mFont(QFont(parentPlot->font().family(), parentPlot->font().pointSize()*1.5, QFont::Bold)),
  mTextColor(Qt::black),
  mSelectedFont(QFont(parentPlot->font().family(), parentPlot->font().pointSize()*1.6, QFont::Bold)),
  mSelectedTextColor(Qt::blue),
  mSelectable(false),
  mSelected(false)
{
  setLayer("axes");
  setMargins(QMargins(5, 5, 5, 0));
}

void QCPPlotTitle::setText(const QString &text)
{
  mText = text;
}

void QCPPlotTitle::setFont(const QFont &font)
{
  mFont = font;
}

void QCPPlotTitle::setTextColor(const QColor &color)
{
  mTextColor = color;
}

void QCPPlotTitle::setSelectedFont(const QFont &font)
{
  mSelectedFont = font;
}

void QCPPlotTitle::setSelectedTextColor(const QColor &color)
{
  mSelectedTextColor = color;
}

void QCPPlotTitle::setSelectable(bool selectable)
{
  if (mSelectable != selectable)
  {
    mSelectable = selectable;
    Q_EMIT selectableChanged(mSelectable);
  }
}

void QCPPlotTitle::setSelected(bool selected)
{
  if (mSelected != selected)
  {
    mSelected = selected;
    Q_EMIT selectionChanged(mSelected);
  }
}

void QCPPlotTitle::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeNone);
}

void QCPPlotTitle::draw(QCPPainter *painter)
{
  painter->setFont(mainFont());
  painter->setPen(QPen(mainTextColor()));
  painter->drawText(mRect, Qt::AlignCenter, mText, &mTextBoundingRect);
}

QSize QCPPlotTitle::minimumSizeHint() const
{
  QFontMetrics metrics(mFont);
  QSize result = metrics.boundingRect(0, 0, 0, 0, Qt::AlignCenter, mText).size();
  result.rwidth() += mMargins.left() + mMargins.right();
  result.rheight() += mMargins.top() + mMargins.bottom();
  return result;
}

QSize QCPPlotTitle::maximumSizeHint() const
{
  QFontMetrics metrics(mFont);
  QSize result = metrics.boundingRect(0, 0, 0, 0, Qt::AlignCenter, mText).size();
  result.rheight() += mMargins.top() + mMargins.bottom();
  result.setWidth(QWIDGETSIZE_MAX);
  return result;
}

void QCPPlotTitle::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  Q_UNUSED(details)
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(additive ? !mSelected : true);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

void QCPPlotTitle::deselectEvent(bool *selectionStateChanged)
{
  if (mSelectable)
  {
    bool selBefore = mSelected;
    setSelected(false);
    if (selectionStateChanged)
      *selectionStateChanged = mSelected != selBefore;
  }
}

double QCPPlotTitle::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  if (mTextBoundingRect.contains(pos.toPoint()))
    return mParentPlot->selectionTolerance()*0.99;
  else
    return -1;
}

QFont QCPPlotTitle::mainFont() const
{
  return mSelected ? mSelectedFont : mFont;
}

QColor QCPPlotTitle::mainTextColor() const
{
  return mSelected ? mSelectedTextColor : mTextColor;
}

QCPColorScale::QCPColorScale(QCustomPlot *parentPlot) :
  QCPLayoutElement(parentPlot),
  mType(QCPAxis::atTop),
  mDataScaleType(QCPAxis::stLinear),
  mBarWidth(20),
  mAxisRect(new QCPColorScaleAxisRectPrivate(this))
{
  setMinimumMargins(QMargins(0, 6, 0, 6));
  setType(QCPAxis::atRight);
  setDataRange(QCPRange(0, 6));
}

QCPColorScale::~QCPColorScale()
{
  delete mAxisRect;
}

QString QCPColorScale::label() const
{
  if (!mColorAxis)
  {
    qDebug() << Q_FUNC_INFO << "internal color axis undefined";
    return QString();
  }

  return mColorAxis.data()->label();
}

bool QCPColorScale::rangeDrag() const
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return false;
  }

  return mAxisRect.data()->rangeDrag().testFlag(QCPAxis::orientation(mType)) &&
      mAxisRect.data()->rangeDragAxis(QCPAxis::orientation(mType)) &&
      mAxisRect.data()->rangeDragAxis(QCPAxis::orientation(mType))->orientation() == QCPAxis::orientation(mType);
}

bool QCPColorScale::rangeZoom() const
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return false;
  }

  return mAxisRect.data()->rangeZoom().testFlag(QCPAxis::orientation(mType)) &&
      mAxisRect.data()->rangeZoomAxis(QCPAxis::orientation(mType)) &&
      mAxisRect.data()->rangeZoomAxis(QCPAxis::orientation(mType))->orientation() == QCPAxis::orientation(mType);
}

void QCPColorScale::setType(QCPAxis::AxisType type)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }
  if (mType != type)
  {
    mType = type;
    QCPRange rangeTransfer(0, 6);
    double logBaseTransfer = 10;
    QString labelTransfer;

    if (mColorAxis)
    {
      rangeTransfer = mColorAxis.data()->range();
      labelTransfer = mColorAxis.data()->label();
      logBaseTransfer = mColorAxis.data()->scaleLogBase();
      mColorAxis.data()->setLabel("");
      disconnect(mColorAxis.data(), SIGNAL(rangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
      disconnect(mColorAxis.data(), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
    }
    Q_FOREACH (QCPAxis::AxisType atype, QList<QCPAxis::AxisType>() << QCPAxis::atLeft << QCPAxis::atRight << QCPAxis::atBottom << QCPAxis::atTop)
    {
      mAxisRect.data()->axis(atype)->setTicks(atype == mType);
      mAxisRect.data()->axis(atype)->setTickLabels(atype== mType);
    }

    mColorAxis = mAxisRect.data()->axis(mType);

    mColorAxis.data()->setRange(rangeTransfer);
    mColorAxis.data()->setLabel(labelTransfer);
    mColorAxis.data()->setScaleLogBase(logBaseTransfer);
    connect(mColorAxis.data(), SIGNAL(rangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
    connect(mColorAxis.data(), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
    mAxisRect.data()->setRangeDragAxes(QCPAxis::orientation(mType) == Qt::Horizontal ? mColorAxis.data() : 0,
                                       QCPAxis::orientation(mType) == Qt::Vertical ? mColorAxis.data() : 0);
  }
}

void QCPColorScale::setDataRange(const QCPRange &dataRange)
{
  if (mDataRange.lower != dataRange.lower || mDataRange.upper != dataRange.upper)
  {
    mDataRange = dataRange;
    if (mColorAxis)
      mColorAxis.data()->setRange(mDataRange);
    Q_EMIT dataRangeChanged(mDataRange);
  }
}

void QCPColorScale::setDataScaleType(QCPAxis::ScaleType scaleType)
{
  if (mDataScaleType != scaleType)
  {
    mDataScaleType = scaleType;
    if (mColorAxis)
      mColorAxis.data()->setScaleType(mDataScaleType);
    if (mDataScaleType == QCPAxis::stLogarithmic)
      setDataRange(mDataRange.sanitizedForLogScale());
    Q_EMIT dataScaleTypeChanged(mDataScaleType);
  }
}

void QCPColorScale::setGradient(const QCPColorGradient &gradient)
{
  if (mGradient != gradient)
  {
    mGradient = gradient;
    if (mAxisRect)
      mAxisRect.data()->mGradientImageInvalidated = true;
    Q_EMIT gradientChanged(mGradient);
  }
}

void QCPColorScale::setLabel(const QString &str)
{
  if (!mColorAxis)
  {
    qDebug() << Q_FUNC_INFO << "internal color axis undefined";
    return;
  }

  mColorAxis.data()->setLabel(str);
}

void QCPColorScale::setBarWidth(int width)
{
  mBarWidth = width;
}

void QCPColorScale::setRangeDrag(bool enabled)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }

  if (enabled)
    mAxisRect.data()->setRangeDrag(QCPAxis::orientation(mType));
  else
    mAxisRect.data()->setRangeDrag(0);
}

void QCPColorScale::setRangeZoom(bool enabled)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }

  if (enabled)
    mAxisRect.data()->setRangeZoom(QCPAxis::orientation(mType));
  else
    mAxisRect.data()->setRangeZoom(0);
}

QList<QCPColorMap*> QCPColorScale::colorMaps() const
{
  QList<QCPColorMap*> result;
  for (int i=0; i<mParentPlot->plottableCount(); ++i)
  {
    if (QCPColorMap *cm = qobject_cast<QCPColorMap*>(mParentPlot->plottable(i)))
      if (cm->colorScale() == this)
        result.append(cm);
  }
  return result;
}

void QCPColorScale::rescaleDataRange(bool onlyVisibleMaps)
{
  QList<QCPColorMap*> maps = colorMaps();
  QCPRange newRange;
  bool haveRange = false;
  int sign = 0;
  if (mDataScaleType == QCPAxis::stLogarithmic)
    sign = (mDataRange.upper < 0 ? -1 : 1);
  for (int i=0; i<maps.size(); ++i)
  {
    if (!maps.at(i)->realVisibility() && onlyVisibleMaps)
      continue;
    QCPRange mapRange;
    if (maps.at(i)->colorScale() == this)
    {
      bool currentFoundRange = true;
      mapRange = maps.at(i)->data()->dataBounds();
      if (sign == 1)
      {
        if (mapRange.lower <= 0 && mapRange.upper > 0)
          mapRange.lower = mapRange.upper*1e-3;
        else if (mapRange.lower <= 0 && mapRange.upper <= 0)
          currentFoundRange = false;
      } else if (sign == -1)
      {
        if (mapRange.upper >= 0 && mapRange.lower < 0)
          mapRange.upper = mapRange.lower*1e-3;
        else if (mapRange.upper >= 0 && mapRange.lower >= 0)
          currentFoundRange = false;
      }
      if (currentFoundRange)
      {
        if (!haveRange)
          newRange = mapRange;
        else
          newRange.expand(mapRange);
        haveRange = true;
      }
    }
  }
  if (haveRange)
  {
    if (!QCPRange::validRange(newRange))
    {
      double center = (newRange.lower+newRange.upper)*0.5;
      if (mDataScaleType == QCPAxis::stLinear)
      {
        newRange.lower = center-mDataRange.size()/2.0;
        newRange.upper = center+mDataRange.size()/2.0;
      } else
      {
        newRange.lower = center/qSqrt(mDataRange.upper/mDataRange.lower);
        newRange.upper = center*qSqrt(mDataRange.upper/mDataRange.lower);
      }
    }
    setDataRange(newRange);
  }
}

void QCPColorScale::update(UpdatePhase phase)
{
  QCPLayoutElement::update(phase);
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }

  mAxisRect.data()->update(phase);

  switch (phase)
  {
    case upMargins:
    {
      if (mType == QCPAxis::atBottom || mType == QCPAxis::atTop)
      {
        setMaximumSize(QWIDGETSIZE_MAX, mBarWidth+mAxisRect.data()->margins().top()+mAxisRect.data()->margins().bottom()+margins().top()+margins().bottom());
        setMinimumSize(0,               mBarWidth+mAxisRect.data()->margins().top()+mAxisRect.data()->margins().bottom()+margins().top()+margins().bottom());
      } else
      {
        setMaximumSize(mBarWidth+mAxisRect.data()->margins().left()+mAxisRect.data()->margins().right()+margins().left()+margins().right(), QWIDGETSIZE_MAX);
        setMinimumSize(mBarWidth+mAxisRect.data()->margins().left()+mAxisRect.data()->margins().right()+margins().left()+margins().right(), 0);
      }
      break;
    }
    case upLayout:
    {
      mAxisRect.data()->setOuterRect(rect());
      break;
    }
    default: break;
  }
}

void QCPColorScale::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  painter->setAntialiasing(false);
}

void QCPColorScale::mousePressEvent(QMouseEvent *event)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }
  mAxisRect.data()->mousePressEvent(event);
}

void QCPColorScale::mouseMoveEvent(QMouseEvent *event)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }
  mAxisRect.data()->mouseMoveEvent(event);
}

void QCPColorScale::mouseReleaseEvent(QMouseEvent *event)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }
  mAxisRect.data()->mouseReleaseEvent(event);
}

void QCPColorScale::wheelEvent(QWheelEvent *event)
{
  if (!mAxisRect)
  {
    qDebug() << Q_FUNC_INFO << "internal axis rect was deleted";
    return;
  }
  mAxisRect.data()->wheelEvent(event);
}

QCPColorScaleAxisRectPrivate::QCPColorScaleAxisRectPrivate(QCPColorScale *parentColorScale) :
  QCPAxisRect(parentColorScale->parentPlot(), true),
  mParentColorScale(parentColorScale),
  mGradientImageInvalidated(true)
{
  setParentLayerable(parentColorScale);
  setMinimumMargins(QMargins(0, 0, 0, 0));
  Q_FOREACH (QCPAxis::AxisType type, QList<QCPAxis::AxisType>() << QCPAxis::atBottom << QCPAxis::atTop << QCPAxis::atLeft << QCPAxis::atRight)
  {
    axis(type)->setVisible(true);
    axis(type)->grid()->setVisible(false);
    axis(type)->setPadding(0);
    connect(axis(type), SIGNAL(selectionChanged(QCPAxis::SelectableParts)), this, SLOT(axisSelectionChanged(QCPAxis::SelectableParts)));
    connect(axis(type), SIGNAL(selectableChanged(QCPAxis::SelectableParts)), this, SLOT(axisSelectableChanged(QCPAxis::SelectableParts)));
  }

  connect(axis(QCPAxis::atLeft), SIGNAL(rangeChanged(QCPRange)), axis(QCPAxis::atRight), SLOT(setRange(QCPRange)));
  connect(axis(QCPAxis::atRight), SIGNAL(rangeChanged(QCPRange)), axis(QCPAxis::atLeft), SLOT(setRange(QCPRange)));
  connect(axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), axis(QCPAxis::atTop), SLOT(setRange(QCPRange)));
  connect(axis(QCPAxis::atTop), SIGNAL(rangeChanged(QCPRange)), axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));
  connect(axis(QCPAxis::atLeft), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), axis(QCPAxis::atRight), SLOT(setScaleType(QCPAxis::ScaleType)));
  connect(axis(QCPAxis::atRight), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), axis(QCPAxis::atLeft), SLOT(setScaleType(QCPAxis::ScaleType)));
  connect(axis(QCPAxis::atBottom), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), axis(QCPAxis::atTop), SLOT(setScaleType(QCPAxis::ScaleType)));
  connect(axis(QCPAxis::atTop), SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)), axis(QCPAxis::atBottom), SLOT(setScaleType(QCPAxis::ScaleType)));

  connect(parentColorScale, SIGNAL(layerChanged(QCPLayer*)), this, SLOT(setLayer(QCPLayer*)));
  Q_FOREACH (QCPAxis::AxisType type, QList<QCPAxis::AxisType>() << QCPAxis::atBottom << QCPAxis::atTop << QCPAxis::atLeft << QCPAxis::atRight)
    connect(parentColorScale, SIGNAL(layerChanged(QCPLayer*)), axis(type), SLOT(setLayer(QCPLayer*)));
}

void QCPColorScaleAxisRectPrivate::draw(QCPPainter *painter)
{
  if (mGradientImageInvalidated)
    updateGradientImage();

  bool mirrorHorz = false;
  bool mirrorVert = false;
  if (mParentColorScale->mColorAxis)
  {
    mirrorHorz = mParentColorScale->mColorAxis.data()->rangeReversed() && (mParentColorScale->type() == QCPAxis::atBottom || mParentColorScale->type() == QCPAxis::atTop);
    mirrorVert = mParentColorScale->mColorAxis.data()->rangeReversed() && (mParentColorScale->type() == QCPAxis::atLeft || mParentColorScale->type() == QCPAxis::atRight);
  }

  painter->drawImage(rect(), mGradientImage.mirrored(mirrorHorz, mirrorVert));
  QCPAxisRect::draw(painter);
}

void QCPColorScaleAxisRectPrivate::updateGradientImage()
{
  if (rect().isEmpty())
    return;

  int n = mParentColorScale->mGradient.levelCount();
  int w, h;
  QVector<double> data(n);
  for (int i=0; i<n; ++i)
    data[i] = i;
  if (mParentColorScale->mType == QCPAxis::atBottom || mParentColorScale->mType == QCPAxis::atTop)
  {
    w = n;
    h = rect().height();
    mGradientImage = QImage(w, h, QImage::Format_RGB32);
    QVector<QRgb*> pixels;
    for (int y=0; y<h; ++y)
      pixels.append(reinterpret_cast<QRgb*>(mGradientImage.scanLine(y)));
    mParentColorScale->mGradient.colorize(data.constData(), QCPRange(0, n-1), pixels.first(), n);
    for (int y=1; y<h; ++y)
      memcpy(pixels.at(y), pixels.first(), n*sizeof(QRgb));
  } else
  {
    w = rect().width();
    h = n;
    mGradientImage = QImage(w, h, QImage::Format_RGB32);
    for (int y=0; y<h; ++y)
    {
      QRgb *pixels = reinterpret_cast<QRgb*>(mGradientImage.scanLine(y));
      const QRgb lineColor = mParentColorScale->mGradient.color(data[h-1-y], QCPRange(0, n-1));
      for (int x=0; x<w; ++x)
        pixels[x] = lineColor;
    }
  }
  mGradientImageInvalidated = false;
}

void QCPColorScaleAxisRectPrivate::axisSelectionChanged(QCPAxis::SelectableParts selectedParts)
{

  Q_FOREACH (QCPAxis::AxisType type, QList<QCPAxis::AxisType>() << QCPAxis::atBottom << QCPAxis::atTop << QCPAxis::atLeft << QCPAxis::atRight)
  {
    if (QCPAxis *senderAxis = qobject_cast<QCPAxis*>(sender()))
      if (senderAxis->axisType() == type)
        continue;

    if (axis(type)->selectableParts().testFlag(QCPAxis::spAxis))
    {
      if (selectedParts.testFlag(QCPAxis::spAxis))
        axis(type)->setSelectedParts(axis(type)->selectedParts() | QCPAxis::spAxis);
      else
        axis(type)->setSelectedParts(axis(type)->selectedParts() & ~QCPAxis::spAxis);
    }
  }
}

void QCPColorScaleAxisRectPrivate::axisSelectableChanged(QCPAxis::SelectableParts selectableParts)
{

  Q_FOREACH (QCPAxis::AxisType type, QList<QCPAxis::AxisType>() << QCPAxis::atBottom << QCPAxis::atTop << QCPAxis::atLeft << QCPAxis::atRight)
  {
    if (QCPAxis *senderAxis = qobject_cast<QCPAxis*>(sender()))
      if (senderAxis->axisType() == type)
        continue;

    if (axis(type)->selectableParts().testFlag(QCPAxis::spAxis))
    {
      if (selectableParts.testFlag(QCPAxis::spAxis))
        axis(type)->setSelectableParts(axis(type)->selectableParts() | QCPAxis::spAxis);
      else
        axis(type)->setSelectableParts(axis(type)->selectableParts() & ~QCPAxis::spAxis);
    }
  }
}

QCPData::QCPData() :
  key(0),
  value(0),
  keyErrorPlus(0),
  keyErrorMinus(0),
  valueErrorPlus(0),
  valueErrorMinus(0)
{
}

QCPData::QCPData(double key, double value) :
  key(key),
  value(value),
  keyErrorPlus(0),
  keyErrorMinus(0),
  valueErrorPlus(0),
  valueErrorMinus(0)
{
}

QCPGraph::QCPGraph(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPAbstractPlottable(keyAxis, valueAxis)
{
  mData = new QCPDataMap;

  setPen(QPen(Qt::blue, 0));
  setErrorPen(QPen(Qt::black));
  setBrush(Qt::NoBrush);
  setSelectedPen(QPen(QColor(80, 80, 255), 2.5));
  setSelectedBrush(Qt::NoBrush);

  setLineStyle(lsLine);
  setErrorType(etNone);
  setErrorBarSize(6);
  setErrorBarSkipSymbol(true);
  setChannelFillGraph(0);
  setAdaptiveSampling(true);
}

QCPGraph::~QCPGraph()
{
  delete mData;
}

void QCPGraph::setData(QCPDataMap *data, bool copy)
{
  if (copy)
  {
    *mData = *data;
  } else
  {
    delete mData;
    mData = data;
  }
}

void QCPGraph::setData(const QVector<double> &key, const QVector<double> &value)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    mData->insertMulti(newData.key, newData);
  }
}

void QCPGraph::setDataValueError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &valueError)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, valueError.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.valueErrorMinus = valueError[i];
    newData.valueErrorPlus = valueError[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setDataValueError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &valueErrorMinus, const QVector<double> &valueErrorPlus)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, valueErrorMinus.size());
  n = qMin(n, valueErrorPlus.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.valueErrorMinus = valueErrorMinus[i];
    newData.valueErrorPlus = valueErrorPlus[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setDataKeyError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &keyError)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, keyError.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.keyErrorMinus = keyError[i];
    newData.keyErrorPlus = keyError[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setDataKeyError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &keyErrorMinus, const QVector<double> &keyErrorPlus)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, keyErrorMinus.size());
  n = qMin(n, keyErrorPlus.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.keyErrorMinus = keyErrorMinus[i];
    newData.keyErrorPlus = keyErrorPlus[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setDataBothError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &keyError, const QVector<double> &valueError)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, valueError.size());
  n = qMin(n, keyError.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.keyErrorMinus = keyError[i];
    newData.keyErrorPlus = keyError[i];
    newData.valueErrorMinus = valueError[i];
    newData.valueErrorPlus = valueError[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setDataBothError(const QVector<double> &key, const QVector<double> &value, const QVector<double> &keyErrorMinus, const QVector<double> &keyErrorPlus, const QVector<double> &valueErrorMinus, const QVector<double> &valueErrorPlus)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  n = qMin(n, valueErrorMinus.size());
  n = qMin(n, valueErrorPlus.size());
  n = qMin(n, keyErrorMinus.size());
  n = qMin(n, keyErrorPlus.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    newData.keyErrorMinus = keyErrorMinus[i];
    newData.keyErrorPlus = keyErrorPlus[i];
    newData.valueErrorMinus = valueErrorMinus[i];
    newData.valueErrorPlus = valueErrorPlus[i];
    mData->insertMulti(key[i], newData);
  }
}

void QCPGraph::setLineStyle(LineStyle ls)
{
  mLineStyle = ls;
}

void QCPGraph::setScatterStyle(const QCPScatterStyle &style)
{
  mScatterStyle = style;
}

void QCPGraph::setErrorType(ErrorType errorType)
{
  mErrorType = errorType;
}

void QCPGraph::setErrorPen(const QPen &pen)
{
  mErrorPen = pen;
}

void QCPGraph::setErrorBarSize(double size)
{
  mErrorBarSize = size;
}

void QCPGraph::setErrorBarSkipSymbol(bool enabled)
{
  mErrorBarSkipSymbol = enabled;
}

void QCPGraph::setChannelFillGraph(QCPGraph *targetGraph)
{

  if (targetGraph == this)
  {
    qDebug() << Q_FUNC_INFO << "targetGraph is this graph itself";
    mChannelFillGraph = 0;
    return;
  }

  if (targetGraph && targetGraph->mParentPlot != mParentPlot)
  {
    qDebug() << Q_FUNC_INFO << "targetGraph not in same plot";
    mChannelFillGraph = 0;
    return;
  }

  mChannelFillGraph = targetGraph;
}

void QCPGraph::setAdaptiveSampling(bool enabled)
{
  mAdaptiveSampling = enabled;
}

void QCPGraph::addData(const QCPDataMap &dataMap)
{
  mData->unite(dataMap);
}

void QCPGraph::addData(const QCPData &data)
{
  mData->insertMulti(data.key, data);
}

void QCPGraph::addData(double key, double value)
{
  QCPData newData;
  newData.key = key;
  newData.value = value;
  mData->insertMulti(newData.key, newData);
}

void QCPGraph::addData(const QVector<double> &keys, const QVector<double> &values)
{
  int n = qMin(keys.size(), values.size());
  QCPData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = keys[i];
    newData.value = values[i];
    mData->insertMulti(newData.key, newData);
  }
}

void QCPGraph::removeDataBefore(double key)
{
  QCPDataMap::iterator it = mData->begin();
  while (it != mData->end() && it.key() < key)
    it = mData->erase(it);
}

void QCPGraph::removeDataAfter(double key)
{
  if (mData->isEmpty()) return;
  QCPDataMap::iterator it = mData->upperBound(key);
  while (it != mData->end())
    it = mData->erase(it);
}

void QCPGraph::removeData(double fromKey, double toKey)
{
  if (fromKey >= toKey || mData->isEmpty()) return;
  QCPDataMap::iterator it = mData->upperBound(fromKey);
  QCPDataMap::iterator itEnd = mData->upperBound(toKey);
  while (it != itEnd)
    it = mData->erase(it);
}

void QCPGraph::removeData(double key)
{
  mData->remove(key);
}

void QCPGraph::clearData()
{
  mData->clear();
}

double QCPGraph::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if ((onlySelectable && !mSelectable) || mData->isEmpty())
    return -1;
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return -1; }

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()))
    return pointDistance(pos);
  else
    return -1;
}

void QCPGraph::rescaleAxes(bool onlyEnlarge, bool includeErrorBars) const
{
  rescaleKeyAxis(onlyEnlarge, includeErrorBars);
  rescaleValueAxis(onlyEnlarge, includeErrorBars);
}

void QCPGraph::rescaleKeyAxis(bool onlyEnlarge, bool includeErrorBars) const
{

  if (mData->isEmpty()) return;

  QCPAxis *keyAxis = mKeyAxis.data();
  if (!keyAxis) { qDebug() << Q_FUNC_INFO << "invalid key axis"; return; }

  SignDomain signDomain = sdBoth;
  if (keyAxis->scaleType() == QCPAxis::stLogarithmic)
    signDomain = (keyAxis->range().upper < 0 ? sdNegative : sdPositive);

  bool foundRange;
  QCPRange newRange = getKeyRange(foundRange, signDomain, includeErrorBars);

  if (foundRange)
  {
    if (onlyEnlarge)
    {
      if (keyAxis->range().lower < newRange.lower)
        newRange.lower = keyAxis->range().lower;
      if (keyAxis->range().upper > newRange.upper)
        newRange.upper = keyAxis->range().upper;
    }
    keyAxis->setRange(newRange);
  }
}

void QCPGraph::rescaleValueAxis(bool onlyEnlarge, bool includeErrorBars) const
{

  if (mData->isEmpty()) return;

  QCPAxis *valueAxis = mValueAxis.data();
  if (!valueAxis) { qDebug() << Q_FUNC_INFO << "invalid value axis"; return; }

  SignDomain signDomain = sdBoth;
  if (valueAxis->scaleType() == QCPAxis::stLogarithmic)
    signDomain = (valueAxis->range().upper < 0 ? sdNegative : sdPositive);

  bool foundRange;
  QCPRange newRange = getValueRange(foundRange, signDomain, includeErrorBars);

  if (foundRange)
  {
    if (onlyEnlarge)
    {
      if (valueAxis->range().lower < newRange.lower)
        newRange.lower = valueAxis->range().lower;
      if (valueAxis->range().upper > newRange.upper)
        newRange.upper = valueAxis->range().upper;
    }
    valueAxis->setRange(newRange);
  }
}

void QCPGraph::draw(QCPPainter *painter)
{
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (mKeyAxis.data()->range().size() <= 0 || mData->isEmpty()) return;
  if (mLineStyle == lsNone && mScatterStyle.isNone()) return;

  QVector<QPointF> *lineData = new QVector<QPointF>;
  QVector<QCPData> *scatterData = 0;
  if (!mScatterStyle.isNone())
    scatterData = new QVector<QCPData>;

  getPlotData(lineData, scatterData);

#ifdef QCUSTOMPLOT_CHECK_DATA
  QCPDataMap::const_iterator it;
  for (it = mData->constBegin(); it != mData->constEnd(); ++it)
  {
    if (QCP::isInvalidData(it.value().key, it.value().value) ||
        QCP::isInvalidData(it.value().keyErrorPlus, it.value().keyErrorMinus) ||
        QCP::isInvalidData(it.value().valueErrorPlus, it.value().valueErrorPlus))
      qDebug() << Q_FUNC_INFO << "Data point at" << it.key() << "invalid." << "Plottable name:" << name();
  }
#endif

  drawFill(painter, lineData);

  if (mLineStyle == lsImpulse)
    drawImpulsePlot(painter, lineData);
  else if (mLineStyle != lsNone)
    drawLinePlot(painter, lineData);

  if (scatterData)
    drawScatterPlot(painter, scatterData);

  delete lineData;
  if (scatterData)
    delete scatterData;
}

void QCPGraph::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{

  if (mBrush.style() != Qt::NoBrush)
  {
    applyFillAntialiasingHint(painter);
    painter->fillRect(QRectF(rect.left(), rect.top()+rect.height()/2.0, rect.width(), rect.height()/3.0), mBrush);
  }

  if (mLineStyle != lsNone)
  {
    applyDefaultAntialiasingHint(painter);
    painter->setPen(mPen);
    painter->drawLine(QLineF(rect.left(), rect.top()+rect.height()/2.0, rect.right()+5, rect.top()+rect.height()/2.0));
  }

  if (!mScatterStyle.isNone())
  {
    applyScattersAntialiasingHint(painter);

    if (mScatterStyle.shape() == QCPScatterStyle::ssPixmap && (mScatterStyle.pixmap().size().width() > rect.width() || mScatterStyle.pixmap().size().height() > rect.height()))
    {
      QCPScatterStyle scaledStyle(mScatterStyle);
      scaledStyle.setPixmap(scaledStyle.pixmap().scaled(rect.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      scaledStyle.applyTo(painter, mPen);
      scaledStyle.drawShape(painter, QRectF(rect).center());
    } else
    {
      mScatterStyle.applyTo(painter, mPen);
      mScatterStyle.drawShape(painter, QRectF(rect).center());
    }
  }
}

void QCPGraph::getPlotData(QVector<QPointF> *lineData, QVector<QCPData> *scatterData) const
{
  switch(mLineStyle)
  {
    case lsNone: getScatterPlotData(scatterData); break;
    case lsLine: getLinePlotData(lineData, scatterData); break;
    case lsStepLeft: getStepLeftPlotData(lineData, scatterData); break;
    case lsStepRight: getStepRightPlotData(lineData, scatterData); break;
    case lsStepCenter: getStepCenterPlotData(lineData, scatterData); break;
    case lsImpulse: getImpulsePlotData(lineData, scatterData); break;
  }
}

void QCPGraph::getScatterPlotData(QVector<QCPData> *scatterData) const
{
  getPreparedData(0, scatterData);
}

void QCPGraph::getLinePlotData(QVector<QPointF> *linePixelData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (!linePixelData) { qDebug() << Q_FUNC_INFO << "null pointer passed as linePixelData"; return; }

  QVector<QCPData> lineData;
  getPreparedData(&lineData, scatterData);
  linePixelData->reserve(lineData.size()+2);
  linePixelData->resize(lineData.size());

  if (keyAxis->orientation() == Qt::Vertical)
  {
    for (int i=0; i<lineData.size(); ++i)
    {
      (*linePixelData)[i].setX(valueAxis->coordToPixel(lineData.at(i).value));
      (*linePixelData)[i].setY(keyAxis->coordToPixel(lineData.at(i).key));
    }
  } else
  {
    for (int i=0; i<lineData.size(); ++i)
    {
      (*linePixelData)[i].setX(keyAxis->coordToPixel(lineData.at(i).key));
      (*linePixelData)[i].setY(valueAxis->coordToPixel(lineData.at(i).value));
    }
  }
}

void QCPGraph::getStepLeftPlotData(QVector<QPointF> *linePixelData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (!linePixelData) { qDebug() << Q_FUNC_INFO << "null pointer passed as lineData"; return; }

  QVector<QCPData> lineData;
  getPreparedData(&lineData, scatterData);
  linePixelData->reserve(lineData.size()*2+2);
  linePixelData->resize(lineData.size()*2);

  if (keyAxis->orientation() == Qt::Vertical)
  {
    double lastValue = valueAxis->coordToPixel(lineData.first().value);
    double key;
    for (int i=0; i<lineData.size(); ++i)
    {
      key = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(lastValue);
      (*linePixelData)[i*2+0].setY(key);
      lastValue = valueAxis->coordToPixel(lineData.at(i).value);
      (*linePixelData)[i*2+1].setX(lastValue);
      (*linePixelData)[i*2+1].setY(key);
    }
  } else
  {
    double lastValue = valueAxis->coordToPixel(lineData.first().value);
    double key;
    for (int i=0; i<lineData.size(); ++i)
    {
      key = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(key);
      (*linePixelData)[i*2+0].setY(lastValue);
      lastValue = valueAxis->coordToPixel(lineData.at(i).value);
      (*linePixelData)[i*2+1].setX(key);
      (*linePixelData)[i*2+1].setY(lastValue);
    }
  }
}

void QCPGraph::getStepRightPlotData(QVector<QPointF> *linePixelData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (!linePixelData) { qDebug() << Q_FUNC_INFO << "null pointer passed as lineData"; return; }

  QVector<QCPData> lineData;
  getPreparedData(&lineData, scatterData);
  linePixelData->reserve(lineData.size()*2+2);
  linePixelData->resize(lineData.size()*2);

  if (keyAxis->orientation() == Qt::Vertical)
  {
    double lastKey = keyAxis->coordToPixel(lineData.first().key);
    double value;
    for (int i=0; i<lineData.size(); ++i)
    {
      value = valueAxis->coordToPixel(lineData.at(i).value);
      (*linePixelData)[i*2+0].setX(value);
      (*linePixelData)[i*2+0].setY(lastKey);
      lastKey = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+1].setX(value);
      (*linePixelData)[i*2+1].setY(lastKey);
    }
  } else
  {
    double lastKey = keyAxis->coordToPixel(lineData.first().key);
    double value;
    for (int i=0; i<lineData.size(); ++i)
    {
      value = valueAxis->coordToPixel(lineData.at(i).value);
      (*linePixelData)[i*2+0].setX(lastKey);
      (*linePixelData)[i*2+0].setY(value);
      lastKey = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+1].setX(lastKey);
      (*linePixelData)[i*2+1].setY(value);
    }
  }
}

void QCPGraph::getStepCenterPlotData(QVector<QPointF> *linePixelData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (!linePixelData) { qDebug() << Q_FUNC_INFO << "null pointer passed as lineData"; return; }

  QVector<QCPData> lineData;
  getPreparedData(&lineData, scatterData);
  linePixelData->reserve(lineData.size()*2+2);
  linePixelData->resize(lineData.size()*2);

  if (keyAxis->orientation() == Qt::Vertical)
  {
    double lastKey = keyAxis->coordToPixel(lineData.first().key);
    double lastValue = valueAxis->coordToPixel(lineData.first().value);
    double key;
    (*linePixelData)[0].setX(lastValue);
    (*linePixelData)[0].setY(lastKey);
    for (int i=1; i<lineData.size(); ++i)
    {
      key = (keyAxis->coordToPixel(lineData.at(i).key)+lastKey)*0.5;
      (*linePixelData)[i*2-1].setX(lastValue);
      (*linePixelData)[i*2-1].setY(key);
      lastValue = valueAxis->coordToPixel(lineData.at(i).value);
      lastKey = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(lastValue);
      (*linePixelData)[i*2+0].setY(key);
    }
    (*linePixelData)[lineData.size()*2-1].setX(lastValue);
    (*linePixelData)[lineData.size()*2-1].setY(lastKey);
  } else
  {
    double lastKey = keyAxis->coordToPixel(lineData.first().key);
    double lastValue = valueAxis->coordToPixel(lineData.first().value);
    double key;
    (*linePixelData)[0].setX(lastKey);
    (*linePixelData)[0].setY(lastValue);
    for (int i=1; i<lineData.size(); ++i)
    {
      key = (keyAxis->coordToPixel(lineData.at(i).key)+lastKey)*0.5;
      (*linePixelData)[i*2-1].setX(key);
      (*linePixelData)[i*2-1].setY(lastValue);
      lastValue = valueAxis->coordToPixel(lineData.at(i).value);
      lastKey = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(key);
      (*linePixelData)[i*2+0].setY(lastValue);
    }
    (*linePixelData)[lineData.size()*2-1].setX(lastKey);
    (*linePixelData)[lineData.size()*2-1].setY(lastValue);
  }

}

void QCPGraph::getImpulsePlotData(QVector<QPointF> *linePixelData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (!linePixelData) { qDebug() << Q_FUNC_INFO << "null pointer passed as linePixelData"; return; }

  QVector<QCPData> lineData;
  getPreparedData(&lineData, scatterData);
  linePixelData->resize(lineData.size()*2);

  if (keyAxis->orientation() == Qt::Vertical)
  {
    double zeroPointX = valueAxis->coordToPixel(0);
    double key;
    for (int i=0; i<lineData.size(); ++i)
    {
      key = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(zeroPointX);
      (*linePixelData)[i*2+0].setY(key);
      (*linePixelData)[i*2+1].setX(valueAxis->coordToPixel(lineData.at(i).value));
      (*linePixelData)[i*2+1].setY(key);
    }
  } else
  {
    double zeroPointY = valueAxis->coordToPixel(0);
    double key;
    for (int i=0; i<lineData.size(); ++i)
    {
      key = keyAxis->coordToPixel(lineData.at(i).key);
      (*linePixelData)[i*2+0].setX(key);
      (*linePixelData)[i*2+0].setY(zeroPointY);
      (*linePixelData)[i*2+1].setX(key);
      (*linePixelData)[i*2+1].setY(valueAxis->coordToPixel(lineData.at(i).value));
    }
  }
}

void QCPGraph::drawFill(QCPPainter *painter, QVector<QPointF> *lineData) const
{
  if (mLineStyle == lsImpulse) return;
  if (mainBrush().style() == Qt::NoBrush || mainBrush().color().alpha() == 0) return;

  applyFillAntialiasingHint(painter);
  if (!mChannelFillGraph)
  {

    addFillBasePoints(lineData);
    painter->setPen(Qt::NoPen);
    painter->setBrush(mainBrush());
    painter->drawPolygon(QPolygonF(*lineData));
    removeFillBasePoints(lineData);
  } else
  {

    painter->setPen(Qt::NoPen);
    painter->setBrush(mainBrush());
    painter->drawPolygon(getChannelFillPolygon(lineData));
  }
}

void QCPGraph::drawScatterPlot(QCPPainter *painter, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  if (mErrorType != etNone)
  {
    applyErrorBarsAntialiasingHint(painter);
    painter->setPen(mErrorPen);
    if (keyAxis->orientation() == Qt::Vertical)
    {
      for (int i=0; i<scatterData->size(); ++i)
        drawError(painter, valueAxis->coordToPixel(scatterData->at(i).value), keyAxis->coordToPixel(scatterData->at(i).key), scatterData->at(i));
    } else
    {
      for (int i=0; i<scatterData->size(); ++i)
        drawError(painter, keyAxis->coordToPixel(scatterData->at(i).key), valueAxis->coordToPixel(scatterData->at(i).value), scatterData->at(i));
    }
  }

  applyScattersAntialiasingHint(painter);
  mScatterStyle.applyTo(painter, mPen);
  if (keyAxis->orientation() == Qt::Vertical)
  {
    for (int i=0; i<scatterData->size(); ++i)
      mScatterStyle.drawShape(painter, valueAxis->coordToPixel(scatterData->at(i).value), keyAxis->coordToPixel(scatterData->at(i).key));
  } else
  {
    for (int i=0; i<scatterData->size(); ++i)
      mScatterStyle.drawShape(painter, keyAxis->coordToPixel(scatterData->at(i).key), valueAxis->coordToPixel(scatterData->at(i).value));
  }
}

void QCPGraph::drawLinePlot(QCPPainter *painter, QVector<QPointF> *lineData) const
{

  if (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)
  {
    applyDefaultAntialiasingHint(painter);
    painter->setPen(mainPen());
    painter->setBrush(Qt::NoBrush);

    if (mParentPlot->plottingHints().testFlag(QCP::phFastPolylines) &&
        painter->pen().style() == Qt::SolidLine &&
        !painter->modes().testFlag(QCPPainter::pmVectorized)&&
        !painter->modes().testFlag(QCPPainter::pmNoCaching))
    {
      for (int i=1; i<lineData->size(); ++i)
        painter->drawLine(lineData->at(i-1), lineData->at(i));
    } else
    {
      painter->drawPolyline(QPolygonF(*lineData));
    }
  }
}

void QCPGraph::drawImpulsePlot(QCPPainter *painter, QVector<QPointF> *lineData) const
{

  if (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)
  {
    applyDefaultAntialiasingHint(painter);
    QPen pen = mainPen();
    pen.setCapStyle(Qt::FlatCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLines(*lineData);
  }
}

void QCPGraph::getPreparedData(QVector<QCPData> *lineData, QVector<QCPData> *scatterData) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  QCPDataMap::const_iterator lower, upper;
  getVisibleDataBounds(lower, upper);
  if (lower == mData->constEnd() || upper == mData->constEnd())
    return;

  int maxCount = std::numeric_limits<int>::max();
  if (mAdaptiveSampling)
  {
    int keyPixelSpan = qAbs(keyAxis->coordToPixel(lower.key())-keyAxis->coordToPixel(upper.key()));
    maxCount = 2*keyPixelSpan+2;
  }
  int dataCount = countDataInBounds(lower, upper, maxCount);

  if (mAdaptiveSampling && dataCount >= maxCount)
  {
    if (lineData)
    {
      QCPDataMap::const_iterator it = lower;
      QCPDataMap::const_iterator upperEnd = upper+1;
      double minValue = it.value().value;
      double maxValue = it.value().value;
      QCPDataMap::const_iterator currentIntervalFirstPoint = it;
      int reversedFactor = keyAxis->rangeReversed() ? -1 : 1;
      int reversedRound = keyAxis->rangeReversed() ? 1 : 0;
      double currentIntervalStartKey = keyAxis->pixelToCoord((int)(keyAxis->coordToPixel(lower.key())+reversedRound));
      double lastIntervalEndKey = currentIntervalStartKey;
      double keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
      bool keyEpsilonVariable = keyAxis->scaleType() == QCPAxis::stLogarithmic;
      int intervalDataCount = 1;
      ++it;
      while (it != upperEnd)
      {
        if (it.key() < currentIntervalStartKey+keyEpsilon)
        {
          if (it.value().value < minValue)
            minValue = it.value().value;
          else if (it.value().value > maxValue)
            maxValue = it.value().value;
          ++intervalDataCount;
        } else
        {
          if (intervalDataCount >= 2)
          {
            if (lastIntervalEndKey < currentIntervalStartKey-keyEpsilon)
              lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.2, currentIntervalFirstPoint.value().value));
            lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.25, minValue));
            lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.75, maxValue));
            if (it.key() > currentIntervalStartKey+keyEpsilon*2)
              lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.8, (it-1).value().value));
          } else
            lineData->append(QCPData(currentIntervalFirstPoint.key(), currentIntervalFirstPoint.value().value));
          lastIntervalEndKey = (it-1).value().key;
          minValue = it.value().value;
          maxValue = it.value().value;
          currentIntervalFirstPoint = it;
          currentIntervalStartKey = keyAxis->pixelToCoord((int)(keyAxis->coordToPixel(it.key())+reversedRound));
          if (keyEpsilonVariable)
            keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
          intervalDataCount = 1;
        }
        ++it;
      }

      if (intervalDataCount >= 2)
      {
        if (lastIntervalEndKey < currentIntervalStartKey-keyEpsilon)
          lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.2, currentIntervalFirstPoint.value().value));
        lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.25, minValue));
        lineData->append(QCPData(currentIntervalStartKey+keyEpsilon*0.75, maxValue));
      } else
        lineData->append(QCPData(currentIntervalFirstPoint.key(), currentIntervalFirstPoint.value().value));
    }

    if (scatterData)
    {
      double valueMaxRange = valueAxis->range().upper;
      double valueMinRange = valueAxis->range().lower;
      QCPDataMap::const_iterator it = lower;
      QCPDataMap::const_iterator upperEnd = upper+1;
      double minValue = it.value().value;
      double maxValue = it.value().value;
      QCPDataMap::const_iterator minValueIt = it;
      QCPDataMap::const_iterator maxValueIt = it;
      QCPDataMap::const_iterator currentIntervalStart = it;
      int reversedFactor = keyAxis->rangeReversed() ? -1 : 1;
      int reversedRound = keyAxis->rangeReversed() ? 1 : 0;
      double currentIntervalStartKey = keyAxis->pixelToCoord((int)(keyAxis->coordToPixel(lower.key())+reversedRound));
      double keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
      bool keyEpsilonVariable = keyAxis->scaleType() == QCPAxis::stLogarithmic;
      int intervalDataCount = 1;
      ++it;
      while (it != upperEnd)
      {
        if (it.key() < currentIntervalStartKey+keyEpsilon)
        {
          if (it.value().value < minValue && it.value().value > valueMinRange && it.value().value < valueMaxRange)
          {
            minValue = it.value().value;
            minValueIt = it;
          } else if (it.value().value > maxValue && it.value().value > valueMinRange && it.value().value < valueMaxRange)
          {
            maxValue = it.value().value;
            maxValueIt = it;
          }
          ++intervalDataCount;
        } else
        {
          if (intervalDataCount >= 2)
          {

            double valuePixelSpan = qAbs(valueAxis->coordToPixel(minValue)-valueAxis->coordToPixel(maxValue));
            int dataModulo = qMax(1, qRound(intervalDataCount/(valuePixelSpan/4.0)));
            QCPDataMap::const_iterator intervalIt = currentIntervalStart;
            int c = 0;
            while (intervalIt != it)
            {
              if ((c % dataModulo == 0 || intervalIt == minValueIt || intervalIt == maxValueIt) && intervalIt.value().value > valueMinRange && intervalIt.value().value < valueMaxRange)
                scatterData->append(intervalIt.value());
              ++c;
              ++intervalIt;
            }
          } else if (currentIntervalStart.value().value > valueMinRange && currentIntervalStart.value().value < valueMaxRange)
            scatterData->append(currentIntervalStart.value());
          minValue = it.value().value;
          maxValue = it.value().value;
          currentIntervalStart = it;
          currentIntervalStartKey = keyAxis->pixelToCoord((int)(keyAxis->coordToPixel(it.key())+reversedRound));
          if (keyEpsilonVariable)
            keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
          intervalDataCount = 1;
        }
        ++it;
      }

      if (intervalDataCount >= 2)
      {

        double valuePixelSpan = qAbs(valueAxis->coordToPixel(minValue)-valueAxis->coordToPixel(maxValue));
        int dataModulo = qMax(1, qRound(intervalDataCount/(valuePixelSpan/4.0)));
        QCPDataMap::const_iterator intervalIt = currentIntervalStart;
        int c = 0;
        while (intervalIt != it)
        {
          if ((c % dataModulo == 0 || intervalIt == minValueIt || intervalIt == maxValueIt) && intervalIt.value().value > valueMinRange && intervalIt.value().value < valueMaxRange)
            scatterData->append(intervalIt.value());
          ++c;
          ++intervalIt;
        }
      } else if (currentIntervalStart.value().value > valueMinRange && currentIntervalStart.value().value < valueMaxRange)
        scatterData->append(currentIntervalStart.value());
    }
  } else
  {
    QVector<QCPData> *dataVector = 0;
    if (lineData)
      dataVector = lineData;
    else if (scatterData)
      dataVector = scatterData;
    if (dataVector)
    {
      QCPDataMap::const_iterator it = lower;
      QCPDataMap::const_iterator upperEnd = upper+1;
      dataVector->reserve(dataCount+2);
      while (it != upperEnd)
      {
        dataVector->append(it.value());
        ++it;
      }
    }
    if (lineData && scatterData)
      *scatterData = *dataVector;
  }
}

void QCPGraph::drawError(QCPPainter *painter, double x, double y, const QCPData &data) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  double a, b;
  double barWidthHalf = mErrorBarSize*0.5;
  double skipSymbolMargin = mScatterStyle.size();

  if (keyAxis->orientation() == Qt::Vertical)
  {

    if (mErrorType == etKey || mErrorType == etBoth)
    {
      a = keyAxis->coordToPixel(data.key-data.keyErrorMinus);
      b = keyAxis->coordToPixel(data.key+data.keyErrorPlus);
      if (keyAxis->rangeReversed())
        qSwap(a,b);

      if (mErrorBarSkipSymbol)
      {
        if (a-y > skipSymbolMargin)
          painter->drawLine(QLineF(x, a, x, y+skipSymbolMargin));
        if (y-b > skipSymbolMargin)
          painter->drawLine(QLineF(x, y-skipSymbolMargin, x, b));
      } else
        painter->drawLine(QLineF(x, a, x, b));

      painter->drawLine(QLineF(x-barWidthHalf, a, x+barWidthHalf, a));
      painter->drawLine(QLineF(x-barWidthHalf, b, x+barWidthHalf, b));
    }
    if (mErrorType == etValue || mErrorType == etBoth)
    {
      a = valueAxis->coordToPixel(data.value-data.valueErrorMinus);
      b = valueAxis->coordToPixel(data.value+data.valueErrorPlus);
      if (valueAxis->rangeReversed())
        qSwap(a,b);

      if (mErrorBarSkipSymbol)
      {
        if (x-a > skipSymbolMargin)
          painter->drawLine(QLineF(a, y, x-skipSymbolMargin, y));
        if (b-x > skipSymbolMargin)
          painter->drawLine(QLineF(x+skipSymbolMargin, y, b, y));
      } else
        painter->drawLine(QLineF(a, y, b, y));

      painter->drawLine(QLineF(a, y-barWidthHalf, a, y+barWidthHalf));
      painter->drawLine(QLineF(b, y-barWidthHalf, b, y+barWidthHalf));
    }
  } else
  {

    if (mErrorType == etKey || mErrorType == etBoth)
    {
      a = keyAxis->coordToPixel(data.key-data.keyErrorMinus);
      b = keyAxis->coordToPixel(data.key+data.keyErrorPlus);
      if (keyAxis->rangeReversed())
        qSwap(a,b);

      if (mErrorBarSkipSymbol)
      {
        if (x-a > skipSymbolMargin)
          painter->drawLine(QLineF(a, y, x-skipSymbolMargin, y));
        if (b-x > skipSymbolMargin)
          painter->drawLine(QLineF(x+skipSymbolMargin, y, b, y));
      } else
        painter->drawLine(QLineF(a, y, b, y));

      painter->drawLine(QLineF(a, y-barWidthHalf, a, y+barWidthHalf));
      painter->drawLine(QLineF(b, y-barWidthHalf, b, y+barWidthHalf));
    }
    if (mErrorType == etValue || mErrorType == etBoth)
    {
      a = valueAxis->coordToPixel(data.value-data.valueErrorMinus);
      b = valueAxis->coordToPixel(data.value+data.valueErrorPlus);
      if (valueAxis->rangeReversed())
        qSwap(a,b);

      if (mErrorBarSkipSymbol)
      {
        if (a-y > skipSymbolMargin)
          painter->drawLine(QLineF(x, a, x, y+skipSymbolMargin));
        if (y-b > skipSymbolMargin)
          painter->drawLine(QLineF(x, y-skipSymbolMargin, x, b));
      } else
        painter->drawLine(QLineF(x, a, x, b));

      painter->drawLine(QLineF(x-barWidthHalf, a, x+barWidthHalf, a));
      painter->drawLine(QLineF(x-barWidthHalf, b, x+barWidthHalf, b));
    }
  }
}

void QCPGraph::getVisibleDataBounds(QCPDataMap::const_iterator &lower, QCPDataMap::const_iterator &upper) const
{
  if (!mKeyAxis) { qDebug() << Q_FUNC_INFO << "invalid key axis"; return; }
  if (mData->isEmpty())
  {
    lower = mData->constEnd();
    upper = mData->constEnd();
    return;
  }

  QCPDataMap::const_iterator lbound = mData->lowerBound(mKeyAxis.data()->range().lower);
  QCPDataMap::const_iterator ubound = mData->upperBound(mKeyAxis.data()->range().upper);
  bool lowoutlier = lbound != mData->constBegin();
  bool highoutlier = ubound != mData->constEnd();

  lower = (lowoutlier ? lbound-1 : lbound);
  upper = (highoutlier ? ubound : ubound-1);
}

int QCPGraph::countDataInBounds(const QCPDataMap::const_iterator &lower, const QCPDataMap::const_iterator &upper, int maxCount) const
{
  if (upper == mData->constEnd() && lower == mData->constEnd())
    return 0;
  QCPDataMap::const_iterator it = lower;
  int count = 1;
  while (it != upper && count < maxCount)
  {
    ++it;
    ++count;
  }
  return count;
}

void QCPGraph::addFillBasePoints(QVector<QPointF> *lineData) const
{
  if (!mKeyAxis) { qDebug() << Q_FUNC_INFO << "invalid key axis"; return; }

  if (mKeyAxis.data()->orientation() == Qt::Vertical)
  {
    *lineData << upperFillBasePoint(lineData->last().y());
    *lineData << lowerFillBasePoint(lineData->first().y());
  } else
  {
    *lineData << upperFillBasePoint(lineData->last().x());
    *lineData << lowerFillBasePoint(lineData->first().x());
  }
}

void QCPGraph::removeFillBasePoints(QVector<QPointF> *lineData) const
{
  lineData->remove(lineData->size()-2, 2);
}

QPointF QCPGraph::lowerFillBasePoint(double lowerKey) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return QPointF(); }

  QPointF point;
  if (valueAxis->scaleType() == QCPAxis::stLinear)
  {
    if (keyAxis->axisType() == QCPAxis::atLeft)
    {
      point.setX(valueAxis->coordToPixel(0));
      point.setY(lowerKey);
    } else if (keyAxis->axisType() == QCPAxis::atRight)
    {
      point.setX(valueAxis->coordToPixel(0));
      point.setY(lowerKey);
    } else if (keyAxis->axisType() == QCPAxis::atTop)
    {
      point.setX(lowerKey);
      point.setY(valueAxis->coordToPixel(0));
    } else if (keyAxis->axisType() == QCPAxis::atBottom)
    {
      point.setX(lowerKey);
      point.setY(valueAxis->coordToPixel(0));
    }
  } else
  {

    if (keyAxis->orientation() == Qt::Vertical)
    {
      if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
          (valueAxis->range().upper > 0 && valueAxis->rangeReversed()))
        point.setX(keyAxis->axisRect()->right());
      else
        point.setX(keyAxis->axisRect()->left());
      point.setY(lowerKey);
    } else if (keyAxis->axisType() == QCPAxis::atTop || keyAxis->axisType() == QCPAxis::atBottom)
    {
      point.setX(lowerKey);
      if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
          (valueAxis->range().upper > 0 && valueAxis->rangeReversed()))
        point.setY(keyAxis->axisRect()->top());
      else
        point.setY(keyAxis->axisRect()->bottom());
    }
  }
  return point;
}

QPointF QCPGraph::upperFillBasePoint(double upperKey) const
{
  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return QPointF(); }

  QPointF point;
  if (valueAxis->scaleType() == QCPAxis::stLinear)
  {
    if (keyAxis->axisType() == QCPAxis::atLeft)
    {
      point.setX(valueAxis->coordToPixel(0));
      point.setY(upperKey);
    } else if (keyAxis->axisType() == QCPAxis::atRight)
    {
      point.setX(valueAxis->coordToPixel(0));
      point.setY(upperKey);
    } else if (keyAxis->axisType() == QCPAxis::atTop)
    {
      point.setX(upperKey);
      point.setY(valueAxis->coordToPixel(0));
    } else if (keyAxis->axisType() == QCPAxis::atBottom)
    {
      point.setX(upperKey);
      point.setY(valueAxis->coordToPixel(0));
    }
  } else
  {

    if (keyAxis->orientation() == Qt::Vertical)
    {
      if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
          (valueAxis->range().upper > 0 && valueAxis->rangeReversed()))
        point.setX(keyAxis->axisRect()->right());
      else
        point.setX(keyAxis->axisRect()->left());
      point.setY(upperKey);
    } else if (keyAxis->axisType() == QCPAxis::atTop || keyAxis->axisType() == QCPAxis::atBottom)
    {
      point.setX(upperKey);
      if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
          (valueAxis->range().upper > 0 && valueAxis->rangeReversed()))
        point.setY(keyAxis->axisRect()->top());
      else
        point.setY(keyAxis->axisRect()->bottom());
    }
  }
  return point;
}

const QPolygonF QCPGraph::getChannelFillPolygon(const QVector<QPointF> *lineData) const
{
  if (!mChannelFillGraph)
    return QPolygonF();

  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return QPolygonF(); }
  if (!mChannelFillGraph.data()->mKeyAxis) { qDebug() << Q_FUNC_INFO << "channel fill target key axis invalid"; return QPolygonF(); }

  if (mChannelFillGraph.data()->mKeyAxis.data()->orientation() != keyAxis->orientation())
    return QPolygonF();

  if (lineData->isEmpty()) return QPolygonF();
  QVector<QPointF> otherData;
  mChannelFillGraph.data()->getPlotData(&otherData, 0);
  if (otherData.isEmpty()) return QPolygonF();
  QVector<QPointF> thisData;
  thisData.reserve(lineData->size()+otherData.size());
  for (int i=0; i<lineData->size(); ++i)
    thisData << lineData->at(i);

  QVector<QPointF> *staticData = &thisData;
  QVector<QPointF> *croppedData = &otherData;

  if (keyAxis->orientation() == Qt::Horizontal)
  {

    if (staticData->first().x() > staticData->last().x())
    {
      int size = staticData->size();
      for (int i=0; i<size/2; ++i)
        qSwap((*staticData)[i], (*staticData)[size-1-i]);
    }
    if (croppedData->first().x() > croppedData->last().x())
    {
      int size = croppedData->size();
      for (int i=0; i<size/2; ++i)
        qSwap((*croppedData)[i], (*croppedData)[size-1-i]);
    }

    if (staticData->first().x() < croppedData->first().x())
      qSwap(staticData, croppedData);
    int lowBound = findIndexBelowX(croppedData, staticData->first().x());
    if (lowBound == -1) return QPolygonF();
    croppedData->remove(0, lowBound);

    if (croppedData->size() < 2) return QPolygonF();
    double slope;
    if (croppedData->at(1).x()-croppedData->at(0).x() != 0)
      slope = (croppedData->at(1).y()-croppedData->at(0).y())/(croppedData->at(1).x()-croppedData->at(0).x());
    else
      slope = 0;
    (*croppedData)[0].setY(croppedData->at(0).y()+slope*(staticData->first().x()-croppedData->at(0).x()));
    (*croppedData)[0].setX(staticData->first().x());

    if (staticData->last().x() > croppedData->last().x())
      qSwap(staticData, croppedData);
    int highBound = findIndexAboveX(croppedData, staticData->last().x());
    if (highBound == -1) return QPolygonF();
    croppedData->remove(highBound+1, croppedData->size()-(highBound+1));

    if (croppedData->size() < 2) return QPolygonF();
    int li = croppedData->size()-1;
    if (croppedData->at(li).x()-croppedData->at(li-1).x() != 0)
      slope = (croppedData->at(li).y()-croppedData->at(li-1).y())/(croppedData->at(li).x()-croppedData->at(li-1).x());
    else
      slope = 0;
    (*croppedData)[li].setY(croppedData->at(li-1).y()+slope*(staticData->last().x()-croppedData->at(li-1).x()));
    (*croppedData)[li].setX(staticData->last().x());
  } else
  {

    if (staticData->first().y() < staticData->last().y())
    {
      int size = staticData->size();
      for (int i=0; i<size/2; ++i)
        qSwap((*staticData)[i], (*staticData)[size-1-i]);
    }
    if (croppedData->first().y() < croppedData->last().y())
    {
      int size = croppedData->size();
      for (int i=0; i<size/2; ++i)
        qSwap((*croppedData)[i], (*croppedData)[size-1-i]);
    }

    if (staticData->first().y() > croppedData->first().y())
      qSwap(staticData, croppedData);
    int lowBound = findIndexAboveY(croppedData, staticData->first().y());
    if (lowBound == -1) return QPolygonF();
    croppedData->remove(0, lowBound);

    if (croppedData->size() < 2) return QPolygonF();
    double slope;
    if (croppedData->at(1).y()-croppedData->at(0).y() != 0)
      slope = (croppedData->at(1).x()-croppedData->at(0).x())/(croppedData->at(1).y()-croppedData->at(0).y());
    else
      slope = 0;
    (*croppedData)[0].setX(croppedData->at(0).x()+slope*(staticData->first().y()-croppedData->at(0).y()));
    (*croppedData)[0].setY(staticData->first().y());

    if (staticData->last().y() < croppedData->last().y())
      qSwap(staticData, croppedData);
    int highBound = findIndexBelowY(croppedData, staticData->last().y());
    if (highBound == -1) return QPolygonF();
    croppedData->remove(highBound+1, croppedData->size()-(highBound+1));

    if (croppedData->size() < 2) return QPolygonF();
    int li = croppedData->size()-1;
    if (croppedData->at(li).y()-croppedData->at(li-1).y() != 0)
      slope = (croppedData->at(li).x()-croppedData->at(li-1).x())/(croppedData->at(li).y()-croppedData->at(li-1).y());
    else
      slope = 0;
    (*croppedData)[li].setX(croppedData->at(li-1).x()+slope*(staticData->last().y()-croppedData->at(li-1).y()));
    (*croppedData)[li].setY(staticData->last().y());
  }

  for (int i=otherData.size()-1; i>=0; --i)
    thisData << otherData.at(i);
  return QPolygonF(thisData);
}

int QCPGraph::findIndexAboveX(const QVector<QPointF> *data, double x) const
{
  for (int i=data->size()-1; i>=0; --i)
  {
    if (data->at(i).x() < x)
    {
      if (i<data->size()-1)
        return i+1;
      else
        return data->size()-1;
    }
  }
  return -1;
}

int QCPGraph::findIndexBelowX(const QVector<QPointF> *data, double x) const
{
  for (int i=0; i<data->size(); ++i)
  {
    if (data->at(i).x() > x)
    {
      if (i>0)
        return i-1;
      else
        return 0;
    }
  }
  return -1;
}

int QCPGraph::findIndexAboveY(const QVector<QPointF> *data, double y) const
{
  for (int i=0; i<data->size(); ++i)
  {
    if (data->at(i).y() < y)
    {
      if (i>0)
        return i-1;
      else
        return 0;
    }
  }
  return -1;
}

double QCPGraph::pointDistance(const QPointF &pixelPoint) const
{
  if (mData->isEmpty())
  {
    qDebug() << Q_FUNC_INFO << "requested point distance on graph" << mName << "without data";
    return 500;
  }
  if (mData->size() == 1)
  {
    QPointF dataPoint = coordsToPixels(mData->constBegin().key(), mData->constBegin().value().value);
    return QVector2D(dataPoint-pixelPoint).length();
  }

  if (mLineStyle == lsNone && mScatterStyle.isNone())
    return 500;

  if (mLineStyle == lsNone)
  {

    QVector<QCPData> *scatterData = new QVector<QCPData>;
    getScatterPlotData(scatterData);
    double minDistSqr = std::numeric_limits<double>::max();
    QPointF ptA;
    QPointF ptB = coordsToPixels(scatterData->at(0).key, scatterData->at(0).value);
    for (int i=1; i<scatterData->size(); ++i)
    {
      ptA = ptB;
      ptB = coordsToPixels(scatterData->at(i).key, scatterData->at(i).value);
      double currentDistSqr = distSqrToLine(ptA, ptB, pixelPoint);
      if (currentDistSqr < minDistSqr)
        minDistSqr = currentDistSqr;
    }
    delete scatterData;
    return sqrt(minDistSqr);
  } else
  {

    QVector<QPointF> *lineData = new QVector<QPointF>;
    getPlotData(lineData, 0);
    double minDistSqr = std::numeric_limits<double>::max();
    if (mLineStyle == lsImpulse)
    {

      for (int i=0; i<lineData->size()-1; i+=2)
      {
        double currentDistSqr = distSqrToLine(lineData->at(i), lineData->at(i+1), pixelPoint);
        if (currentDistSqr < minDistSqr)
          minDistSqr = currentDistSqr;
      }
    } else
    {

      for (int i=0; i<lineData->size()-1; ++i)
      {
        double currentDistSqr = distSqrToLine(lineData->at(i), lineData->at(i+1), pixelPoint);
        if (currentDistSqr < minDistSqr)
          minDistSqr = currentDistSqr;
      }
    }
    delete lineData;
    return sqrt(minDistSqr);
  }
}

int QCPGraph::findIndexBelowY(const QVector<QPointF> *data, double y) const
{
  for (int i=data->size()-1; i>=0; --i)
  {
    if (data->at(i).y() > y)
    {
      if (i<data->size()-1)
        return i+1;
      else
        return data->size()-1;
    }
  }
  return -1;
}

QCPRange QCPGraph::getKeyRange(bool &foundRange, SignDomain inSignDomain) const
{

  return getKeyRange(foundRange, inSignDomain, true);
}

QCPRange QCPGraph::getValueRange(bool &foundRange, SignDomain inSignDomain) const
{

  return getValueRange(foundRange, inSignDomain, true);
}

QCPRange QCPGraph::getKeyRange(bool &foundRange, SignDomain inSignDomain, bool includeErrors) const
{
  QCPRange range;
  bool haveLower = false;
  bool haveUpper = false;

  double current, currentErrorMinus, currentErrorPlus;

  if (inSignDomain == sdBoth)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().key;
      currentErrorMinus = (includeErrors ? it.value().keyErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().keyErrorPlus : 0);
      if (current-currentErrorMinus < range.lower || !haveLower)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if (current+currentErrorPlus > range.upper || !haveUpper)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      ++it;
    }
  } else if (inSignDomain == sdNegative)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().key;
      currentErrorMinus = (includeErrors ? it.value().keyErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().keyErrorPlus : 0);
      if ((current-currentErrorMinus < range.lower || !haveLower) && current-currentErrorMinus < 0)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if ((current+currentErrorPlus > range.upper || !haveUpper) && current+currentErrorPlus < 0)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      if (includeErrors)
      {
        if ((current < range.lower || !haveLower) && current < 0)
        {
          range.lower = current;
          haveLower = true;
        }
        if ((current > range.upper || !haveUpper) && current < 0)
        {
          range.upper = current;
          haveUpper = true;
        }
      }
      ++it;
    }
  } else if (inSignDomain == sdPositive)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().key;
      currentErrorMinus = (includeErrors ? it.value().keyErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().keyErrorPlus : 0);
      if ((current-currentErrorMinus < range.lower || !haveLower) && current-currentErrorMinus > 0)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if ((current+currentErrorPlus > range.upper || !haveUpper) && current+currentErrorPlus > 0)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      if (includeErrors)
      {
        if ((current < range.lower || !haveLower) && current > 0)
        {
          range.lower = current;
          haveLower = true;
        }
        if ((current > range.upper || !haveUpper) && current > 0)
        {
          range.upper = current;
          haveUpper = true;
        }
      }
      ++it;
    }
  }

  foundRange = haveLower && haveUpper;
  return range;
}

QCPRange QCPGraph::getValueRange(bool &foundRange, SignDomain inSignDomain, bool includeErrors) const
{
  QCPRange range;
  bool haveLower = false;
  bool haveUpper = false;

  double current, currentErrorMinus, currentErrorPlus;

  if (inSignDomain == sdBoth)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().value;
      currentErrorMinus = (includeErrors ? it.value().valueErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().valueErrorPlus : 0);
      if (current-currentErrorMinus < range.lower || !haveLower)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if (current+currentErrorPlus > range.upper || !haveUpper)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      ++it;
    }
  } else if (inSignDomain == sdNegative)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().value;
      currentErrorMinus = (includeErrors ? it.value().valueErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().valueErrorPlus : 0);
      if ((current-currentErrorMinus < range.lower || !haveLower) && current-currentErrorMinus < 0)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if ((current+currentErrorPlus > range.upper || !haveUpper) && current+currentErrorPlus < 0)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      if (includeErrors)
      {
        if ((current < range.lower || !haveLower) && current < 0)
        {
          range.lower = current;
          haveLower = true;
        }
        if ((current > range.upper || !haveUpper) && current < 0)
        {
          range.upper = current;
          haveUpper = true;
        }
      }
      ++it;
    }
  } else if (inSignDomain == sdPositive)
  {
    QCPDataMap::const_iterator it = mData->constBegin();
    while (it != mData->constEnd())
    {
      current = it.value().value;
      currentErrorMinus = (includeErrors ? it.value().valueErrorMinus : 0);
      currentErrorPlus = (includeErrors ? it.value().valueErrorPlus : 0);
      if ((current-currentErrorMinus < range.lower || !haveLower) && current-currentErrorMinus > 0)
      {
        range.lower = current-currentErrorMinus;
        haveLower = true;
      }
      if ((current+currentErrorPlus > range.upper || !haveUpper) && current+currentErrorPlus > 0)
      {
        range.upper = current+currentErrorPlus;
        haveUpper = true;
      }
      if (includeErrors)
      {
        if ((current < range.lower || !haveLower) && current > 0)
        {
          range.lower = current;
          haveLower = true;
        }
        if ((current > range.upper || !haveUpper) && current > 0)
        {
          range.upper = current;
          haveUpper = true;
        }
      }
      ++it;
    }
  }

  foundRange = haveLower && haveUpper;
  return range;
}

QCPCurveData::QCPCurveData() :
  t(0),
  key(0),
  value(0)
{
}

QCPCurveData::QCPCurveData(double t, double key, double value) :
  t(t),
  key(key),
  value(value)
{
}

QCPCurve::QCPCurve(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPAbstractPlottable(keyAxis, valueAxis)
{
  mData = new QCPCurveDataMap;
  mPen.setColor(Qt::blue);
  mPen.setStyle(Qt::SolidLine);
  mBrush.setColor(Qt::blue);
  mBrush.setStyle(Qt::NoBrush);
  mSelectedPen = mPen;
  mSelectedPen.setWidthF(2.5);
  mSelectedPen.setColor(QColor(80, 80, 255));
  mSelectedBrush = mBrush;

  setScatterStyle(QCPScatterStyle());
  setLineStyle(lsLine);
}

QCPCurve::~QCPCurve()
{
  delete mData;
}

void QCPCurve::setData(QCPCurveDataMap *data, bool copy)
{
  if (copy)
  {
    *mData = *data;
  } else
  {
    delete mData;
    mData = data;
  }
}

void QCPCurve::setData(const QVector<double> &t, const QVector<double> &key, const QVector<double> &value)
{
  mData->clear();
  int n = t.size();
  n = qMin(n, key.size());
  n = qMin(n, value.size());
  QCPCurveData newData;
  for (int i=0; i<n; ++i)
  {
    newData.t = t[i];
    newData.key = key[i];
    newData.value = value[i];
    mData->insertMulti(newData.t, newData);
  }
}

void QCPCurve::setData(const QVector<double> &key, const QVector<double> &value)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  QCPCurveData newData;
  for (int i=0; i<n; ++i)
  {
    newData.t = i;
    newData.key = key[i];
    newData.value = value[i];
    mData->insertMulti(newData.t, newData);
  }
}

void QCPCurve::setScatterStyle(const QCPScatterStyle &style)
{
  mScatterStyle = style;
}

void QCPCurve::setLineStyle(QCPCurve::LineStyle style)
{
  mLineStyle = style;
}

void QCPCurve::addData(const QCPCurveDataMap &dataMap)
{
  mData->unite(dataMap);
}

void QCPCurve::addData(const QCPCurveData &data)
{
  mData->insertMulti(data.t, data);
}

void QCPCurve::addData(double t, double key, double value)
{
  QCPCurveData newData;
  newData.t = t;
  newData.key = key;
  newData.value = value;
  mData->insertMulti(newData.t, newData);
}

void QCPCurve::addData(double key, double value)
{
  QCPCurveData newData;
  if (!mData->isEmpty())
    newData.t = (mData->constEnd()-1).key()+1;
  else
    newData.t = 0;
  newData.key = key;
  newData.value = value;
  mData->insertMulti(newData.t, newData);
}

void QCPCurve::addData(const QVector<double> &ts, const QVector<double> &keys, const QVector<double> &values)
{
  int n = ts.size();
  n = qMin(n, keys.size());
  n = qMin(n, values.size());
  QCPCurveData newData;
  for (int i=0; i<n; ++i)
  {
    newData.t = ts[i];
    newData.key = keys[i];
    newData.value = values[i];
    mData->insertMulti(newData.t, newData);
  }
}

void QCPCurve::removeDataBefore(double t)
{
  QCPCurveDataMap::iterator it = mData->begin();
  while (it != mData->end() && it.key() < t)
    it = mData->erase(it);
}

void QCPCurve::removeDataAfter(double t)
{
  if (mData->isEmpty()) return;
  QCPCurveDataMap::iterator it = mData->upperBound(t);
  while (it != mData->end())
    it = mData->erase(it);
}

void QCPCurve::removeData(double fromt, double tot)
{
  if (fromt >= tot || mData->isEmpty()) return;
  QCPCurveDataMap::iterator it = mData->upperBound(fromt);
  QCPCurveDataMap::iterator itEnd = mData->upperBound(tot);
  while (it != itEnd)
    it = mData->erase(it);
}

void QCPCurve::removeData(double t)
{
  mData->remove(t);
}

void QCPCurve::clearData()
{
  mData->clear();
}

double QCPCurve::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if ((onlySelectable && !mSelectable) || mData->isEmpty())
    return -1;
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return -1; }

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()))
    return pointDistance(pos);
  else
    return -1;
}

void QCPCurve::draw(QCPPainter *painter)
{
  if (mData->isEmpty()) return;

  QVector<QPointF> *lineData = new QVector<QPointF>;

  getCurveData(lineData);

#ifdef QCUSTOMPLOT_CHECK_DATA
  QCPCurveDataMap::const_iterator it;
  for (it = mData->constBegin(); it != mData->constEnd(); ++it)
  {
    if (QCP::isInvalidData(it.value().t) ||
        QCP::isInvalidData(it.value().key, it.value().value))
      qDebug() << Q_FUNC_INFO << "Data point at" << it.key() << "invalid." << "Plottable name:" << name();
  }
#endif

  if (mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0)
  {
    applyFillAntialiasingHint(painter);
    painter->setPen(Qt::NoPen);
    painter->setBrush(mainBrush());
    painter->drawPolygon(QPolygonF(*lineData));
  }

  if (mLineStyle != lsNone && mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)
  {
    applyDefaultAntialiasingHint(painter);
    painter->setPen(mainPen());
    painter->setBrush(Qt::NoBrush);

    if (mParentPlot->plottingHints().testFlag(QCP::phFastPolylines) &&
        painter->pen().style() == Qt::SolidLine &&
        !painter->modes().testFlag(QCPPainter::pmVectorized) &&
        !painter->modes().testFlag(QCPPainter::pmNoCaching))
    {
      for (int i=1; i<lineData->size(); ++i)
        painter->drawLine(lineData->at(i-1), lineData->at(i));
    } else
    {
      painter->drawPolyline(QPolygonF(*lineData));
    }
  }

  if (!mScatterStyle.isNone())
    drawScatterPlot(painter, lineData);

  delete lineData;
}

void QCPCurve::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{

  if (mBrush.style() != Qt::NoBrush)
  {
    applyFillAntialiasingHint(painter);
    painter->fillRect(QRectF(rect.left(), rect.top()+rect.height()/2.0, rect.width(), rect.height()/3.0), mBrush);
  }

  if (mLineStyle != lsNone)
  {
    applyDefaultAntialiasingHint(painter);
    painter->setPen(mPen);
    painter->drawLine(QLineF(rect.left(), rect.top()+rect.height()/2.0, rect.right()+5, rect.top()+rect.height()/2.0));
  }

  if (!mScatterStyle.isNone())
  {
    applyScattersAntialiasingHint(painter);

    if (mScatterStyle.shape() == QCPScatterStyle::ssPixmap && (mScatterStyle.pixmap().size().width() > rect.width() || mScatterStyle.pixmap().size().height() > rect.height()))
    {
      QCPScatterStyle scaledStyle(mScatterStyle);
      scaledStyle.setPixmap(scaledStyle.pixmap().scaled(rect.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      scaledStyle.applyTo(painter, mPen);
      scaledStyle.drawShape(painter, QRectF(rect).center());
    } else
    {
      mScatterStyle.applyTo(painter, mPen);
      mScatterStyle.drawShape(painter, QRectF(rect).center());
    }
  }
}

void QCPCurve::drawScatterPlot(QCPPainter *painter, const QVector<QPointF> *pointData) const
{

  applyScattersAntialiasingHint(painter);
  mScatterStyle.applyTo(painter, mPen);
  for (int i=0; i<pointData->size(); ++i)
    mScatterStyle.drawShape(painter,  pointData->at(i));
}

void QCPCurve::getCurveData(QVector<QPointF> *lineData) const
{

  QCPAxis *keyAxis = mKeyAxis.data();
  QCPAxis *valueAxis = mValueAxis.data();
  if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

  QRect axisRect = mKeyAxis.data()->axisRect()->rect() & mValueAxis.data()->axisRect()->rect();
  lineData->reserve(mData->size());
  QCPCurveDataMap::const_iterator it;
  int lastRegion = 5;
  int currentRegion = 5;
  double RLeft = keyAxis->range().lower;
  double RRight = keyAxis->range().upper;
  double RBottom = valueAxis->range().lower;
  double RTop = valueAxis->range().upper;
  double x, y;
  bool addedLastAlready = true;
  bool firstPoint = true;
  for (it = mData->constBegin(); it != mData->constEnd(); ++it)
  {
    x = it.value().key;
    y = it.value().value;

    if (x < RLeft)
    {
      if (y > RTop)
        currentRegion = 1;
      else if (y < RBottom)
        currentRegion = 3;
      else
        currentRegion = 2;
    } else if (x > RRight)
    {
      if (y > RTop)
        currentRegion = 7;
      else if (y < RBottom)
        currentRegion = 9;
      else
        currentRegion = 8;
    } else
    {
      if (y > RTop)
        currentRegion = 4;
      else if (y < RBottom)
        currentRegion = 6;
      else
        currentRegion = 5;
    }

    if (currentRegion == 5 || (firstPoint && mBrush.style() != Qt::NoBrush))
    {
      if (!addedLastAlready)
        lineData->append(coordsToPixels((it-1).value().key, (it-1).value().value));
      else if (lastRegion != 5)
      {
        if (!firstPoint)
          lineData->replace(lineData->size()-1, coordsToPixels((it-1).value().key, (it-1).value().value));
      }
      lineData->append(coordsToPixels(it.value().key, it.value().value));
      addedLastAlready = true;
    } else if (currentRegion != lastRegion)
    {

      if (lastRegion == 5 ||
          ((lastRegion==2 && currentRegion==4) || (lastRegion==4 && currentRegion==2)) ||
          ((lastRegion==4 && currentRegion==8) || (lastRegion==8 && currentRegion==4)) ||
          ((lastRegion==8 && currentRegion==6) || (lastRegion==6 && currentRegion==8)) ||
          ((lastRegion==6 && currentRegion==2) || (lastRegion==2 && currentRegion==6))
          )
      {

        if (!addedLastAlready)
          lineData->append(coordsToPixels((it-1).value().key, (it-1).value().value));

        lineData->append(coordsToPixels(it.value().key, it.value().value));
      } else
      {

        if (!addedLastAlready)
          lineData->append(outsideCoordsToPixels((it-1).value().key, (it-1).value().value, currentRegion, axisRect));

        lineData->append(outsideCoordsToPixels(it.value().key, it.value().value, currentRegion, axisRect));
      }
      addedLastAlready = true;
    } else
    {
      addedLastAlready = false;
    }
    lastRegion = currentRegion;
    firstPoint = false;
  }

  if (lastRegion != 5 && mBrush.style() != Qt::NoBrush && !mData->isEmpty())
    lineData->append(coordsToPixels((mData->constEnd()-1).value().key, (mData->constEnd()-1).value().value));
}

double QCPCurve::pointDistance(const QPointF &pixelPoint) const
{
  if (mData->isEmpty())
  {
    qDebug() << Q_FUNC_INFO << "requested point distance on curve" << mName << "without data";
    return 500;
  }
  if (mData->size() == 1)
  {
    QPointF dataPoint = coordsToPixels(mData->constBegin().key(), mData->constBegin().value().value);
    return QVector2D(dataPoint-pixelPoint).length();
  }

  QVector<QPointF> *lineData = new QVector<QPointF>;
  getCurveData(lineData);
  double minDistSqr = std::numeric_limits<double>::max();
  for (int i=0; i<lineData->size()-1; ++i)
  {
    double currentDistSqr = distSqrToLine(lineData->at(i), lineData->at(i+1), pixelPoint);
    if (currentDistSqr < minDistSqr)
      minDistSqr = currentDistSqr;
  }
  delete lineData;
  return sqrt(minDistSqr);
}

QPointF QCPCurve::outsideCoordsToPixels(double key, double value, int region, QRect axisRect) const
{
  int margin = qCeil(qMax(mScatterStyle.size(), (double)mPen.widthF())) + 2;
  QPointF result = coordsToPixels(key, value);
  switch (region)
  {
    case 2: result.setX(axisRect.left()-margin); break;
    case 8: result.setX(axisRect.right()+margin); break;
    case 4: result.setY(axisRect.top()-margin); break;
    case 6: result.setY(axisRect.bottom()+margin); break;
    case 1: result.setX(axisRect.left()-margin);
            result.setY(axisRect.top()-margin); break;
    case 7: result.setX(axisRect.right()+margin);
            result.setY(axisRect.top()-margin); break;
    case 9: result.setX(axisRect.right()+margin);
            result.setY(axisRect.bottom()+margin); break;
    case 3: result.setX(axisRect.left()-margin);
            result.setY(axisRect.bottom()+margin); break;
  }
  return result;
}

QCPRange QCPCurve::getKeyRange(bool &foundRange, SignDomain inSignDomain) const
{
  QCPRange range;
  bool haveLower = false;
  bool haveUpper = false;

  double current;

  QCPCurveDataMap::const_iterator it = mData->constBegin();
  while (it != mData->constEnd())
  {
    current = it.value().key;
    if (inSignDomain == sdBoth || (inSignDomain == sdNegative && current < 0) || (inSignDomain == sdPositive && current > 0))
    {
      if (current < range.lower || !haveLower)
      {
        range.lower = current;
        haveLower = true;
      }
      if (current > range.upper || !haveUpper)
      {
        range.upper = current;
        haveUpper = true;
      }
    }
    ++it;
  }

  foundRange = haveLower && haveUpper;
  return range;
}

QCPRange QCPCurve::getValueRange(bool &foundRange, SignDomain inSignDomain) const
{
  QCPRange range;
  bool haveLower = false;
  bool haveUpper = false;

  double current;

  QCPCurveDataMap::const_iterator it = mData->constBegin();
  while (it != mData->constEnd())
  {
    current = it.value().value;
    if (inSignDomain == sdBoth || (inSignDomain == sdNegative && current < 0) || (inSignDomain == sdPositive && current > 0))
    {
      if (current < range.lower || !haveLower)
      {
        range.lower = current;
        haveLower = true;
      }
      if (current > range.upper || !haveUpper)
      {
        range.upper = current;
        haveUpper = true;
      }
    }
    ++it;
  }

  foundRange = haveLower && haveUpper;
  return range;
}

QCPBarData::QCPBarData() :
  key(0),
  value(0)
{
}

QCPBarData::QCPBarData(double key, double value) :
  key(key),
  value(value)
{
}

QCPBars::QCPBars(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPAbstractPlottable(keyAxis, valueAxis)
{
  mData = new QCPBarDataMap;
  mPen.setColor(Qt::blue);
  mPen.setStyle(Qt::SolidLine);
  mBrush.setColor(QColor(40, 50, 255, 30));
  mBrush.setStyle(Qt::SolidPattern);
  mSelectedPen = mPen;
  mSelectedPen.setWidthF(2.5);
  mSelectedPen.setColor(QColor(80, 80, 255));
  mSelectedBrush = mBrush;

  mWidth = 0.75;
}

QCPBars::~QCPBars()
{
  if (mBarBelow || mBarAbove)
    connectBars(mBarBelow.data(), mBarAbove.data());
  delete mData;
}

void QCPBars::setWidth(double width)
{
  mWidth = width;
}

void QCPBars::setData(QCPBarDataMap *data, bool copy)
{
  if (copy)
  {
    *mData = *data;
  } else
  {
    delete mData;
    mData = data;
  }
}

void QCPBars::setData(const QVector<double> &key, const QVector<double> &value)
{
  mData->clear();
  int n = key.size();
  n = qMin(n, value.size());
  QCPBarData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = key[i];
    newData.value = value[i];
    mData->insertMulti(newData.key, newData);
  }
}

void QCPBars::moveBelow(QCPBars *bars)
{
  if (bars == this) return;
  if (bars && (bars->keyAxis() != mKeyAxis.data() || bars->valueAxis() != mValueAxis.data()))
  {
    qDebug() << Q_FUNC_INFO << "passed QCPBars* doesn't have same key and value axis as this QCPBars";
    return;
  }

  connectBars(mBarBelow.data(), mBarAbove.data());

  if (bars)
  {
    if (bars->mBarBelow)
      connectBars(bars->mBarBelow.data(), this);
    connectBars(this, bars);
  }
}

void QCPBars::moveAbove(QCPBars *bars)
{
  if (bars == this) return;
  if (bars && (bars->keyAxis() != mKeyAxis.data() || bars->valueAxis() != mValueAxis.data()))
  {
    qDebug() << Q_FUNC_INFO << "passed QCPBars* doesn't have same key and value axis as this QCPBars";
    return;
  }

  connectBars(mBarBelow.data(), mBarAbove.data());

  if (bars)
  {
    if (bars->mBarAbove)
      connectBars(this, bars->mBarAbove.data());
    connectBars(bars, this);
  }
}

void QCPBars::addData(const QCPBarDataMap &dataMap)
{
  mData->unite(dataMap);
}

void QCPBars::addData(const QCPBarData &data)
{
  mData->insertMulti(data.key, data);
}

void QCPBars::addData(double key, double value)
{
  QCPBarData newData;
  newData.key = key;
  newData.value = value;
  mData->insertMulti(newData.key, newData);
}

void QCPBars::addData(const QVector<double> &keys, const QVector<double> &values)
{
  int n = keys.size();
  n = qMin(n, values.size());
  QCPBarData newData;
  for (int i=0; i<n; ++i)
  {
    newData.key = keys[i];
    newData.value = values[i];
    mData->insertMulti(newData.key, newData);
  }
}

void QCPBars::removeDataBefore(double key)
{
  QCPBarDataMap::iterator it = mData->begin();
  while (it != mData->end() && it.key() < key)
    it = mData->erase(it);
}

void QCPBars::removeDataAfter(double key)
{
  if (mData->isEmpty()) return;
  QCPBarDataMap::iterator it = mData->upperBound(key);
  while (it != mData->end())
    it = mData->erase(it);
}

void QCPBars::removeData(double fromKey, double toKey)
{
  if (fromKey >= toKey || mData->isEmpty()) return;
  QCPBarDataMap::iterator it = mData->upperBound(fromKey);
  QCPBarDataMap::iterator itEnd = mData->upperBound(toKey);
  while (it != itEnd)
    it = mData->erase(it);
}

void QCPBars::removeData(double key)
{
  mData->remove(key);
}

void QCPBars::clearData()
{
  mData->clear();
}

double QCPBars::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return -1; }

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()))
  {
    QCPBarDataMap::ConstIterator it;
    double posKey, posValue;
    pixelsToCoords(pos, posKey, posValue);
    for (it = mData->constBegin(); it != mData->constEnd(); ++it)
    {
      double baseValue = getBaseValue(it.key(), it.value().value >=0);
      QCPRange keyRange(it.key()-mWidth*0.5, it.key()+mWidth*0.5);
      QCPRange valueRange(baseValue, baseValue+it.value().value);
      if (keyRange.contains(posKey) && valueRange.contains(posValue))
        return mParentPlot->selectionTolerance()*0.99;
    }
  }
  return -1;
}

void QCPBars::draw(QCPPainter *painter)
{
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
  if (mData->isEmpty()) return;

  QCPBarDataMap::const_iterator it;
  for (it = mData->constBegin(); it != mData->constEnd(); ++it)
  {

    if (it.key()+mWidth*0.5 < mKeyAxis.data()->range().lower || it.key()-mWidth*0.5 > mKeyAxis.data()->range().upper)
      continue;

#ifdef QCUSTOMPLOT_CHECK_DATA
    if (QCP::isInvalidData(it.value().key, it.value().value))
      qDebug() << Q_FUNC_INFO << "Data point at" << it.key() << "of drawn range invalid." << "Plottable name:" << name();
#endif
    QPolygonF barPolygon = getBarPolygon(it.key(), it.value().value);

    if (mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0)
    {
      applyFillAntialiasingHint(painter);
      painter->setPen(Qt::NoPen);
      painter->setBrush(mainBrush());
      painter->drawPolygon(barPolygon);
    }

    if (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)
    {
      applyDefaultAntialiasingHint(painter);
      painter->setPen(mainPen());
      painter->setBrush(Qt::NoBrush);
      painter->drawPolyline(barPolygon);
    }
  }
}

void QCPBars::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{

  applyDefaultAntialiasingHint(painter);
  painter->setBrush(mBrush);
  painter->setPen(mPen);
  QRectF r = QRectF(0, 0, rect.width()*0.67, rect.height()*0.67);
  r.moveCenter(rect.center());
  painter->drawRect(r);
}

QPolygonF QCPBars::getBarPolygon(double key, double value) const
{
  QPolygonF result;
  double baseValue = getBaseValue(key, value >= 0);
  result << coordsToPixels(key-mWidth*0.5, baseValue);
  result << coordsToPixels(key-mWidth*0.5, baseValue+value);
  result << coordsToPixels(key+mWidth*0.5, baseValue+value);
  result << coordsToPixels(key+mWidth*0.5, baseValue);
  return result;
}

double QCPBars::getBaseValue(double key, bool positive) const
{
  if (mBarBelow)
  {
    double max = 0;

    QCPBarDataMap::const_iterator it = mBarBelow.data()->mData->lowerBound(key-mWidth*0.1);
    QCPBarDataMap::const_iterator itEnd = mBarBelow.data()->mData->upperBound(key+mWidth*0.1);
    while (it != itEnd)
    {
      if ((positive && it.value().value > max) ||
          (!positive && it.value().value < max))
        max = it.value().value;
      ++it;
    }

    return max + mBarBelow.data()->getBaseValue(key, positive);
  } else
    return 0;
}

void QCPBars::connectBars(QCPBars *lower, QCPBars *upper)
{
  if (!lower && !upper) return;

  if (!lower)
  {

    if (upper->mBarBelow && upper->mBarBelow.data()->mBarAbove.data() == upper)
      upper->mBarBelow.data()->mBarAbove = 0;
    upper->mBarBelow = 0;
  } else if (!upper)
  {

    if (lower->mBarAbove && lower->mBarAbove.data()->mBarBelow.data() == lower)
      lower->mBarAbove.data()->mBarBelow = 0;
    lower->mBarAbove = 0;
  } else
  {

    if (lower->mBarAbove && lower->mBarAbove.data()->mBarBelow.data() == lower)
      lower->mBarAbove.data()->mBarBelow = 0;

    if (upper->mBarBelow && upper->mBarBelow.data()->mBarAbove.data() == upper)
      upper->mBarBelow.data()->mBarAbove = 0;
    lower->mBarAbove = upper;
    upper->mBarBelow = lower;
  }
}

QCPRange QCPBars::getKeyRange(bool &foundRange, SignDomain inSignDomain) const
{
  QCPRange range;
  bool haveLower = false;
  bool haveUpper = false;

  double current;
  double barWidthHalf = mWidth*0.5;
  QCPBarDataMap::const_iterator it = mData->constBegin();
  while (it != mData->constEnd())
  {
    current = it.value().key;
    if (inSignDomain == sdBoth || (inSignDomain == sdNegative && current+barWidthHalf < 0) || (inSignDomain == sdPositive && current-barWidthHalf > 0))
    {
      if (current-barWidthHalf < range.lower || !haveLower)
      {
        range.lower = current-barWidthHalf;
        haveLower = true;
      }
      if (current+barWidthHalf > range.upper || !haveUpper)
      {
        range.upper = current+barWidthHalf;
        haveUpper = true;
      }
    }
    ++it;
  }

  foundRange = haveLower && haveUpper;
  return range;
}

QCPRange QCPBars::getValueRange(bool &foundRange, SignDomain inSignDomain) const
{
  QCPRange range;
  bool haveLower = true;
  bool haveUpper = true;

  double current;

  QCPBarDataMap::const_iterator it = mData->constBegin();
  while (it != mData->constEnd())
  {
    current = it.value().value + getBaseValue(it.value().key, it.value().value >= 0);
    if (inSignDomain == sdBoth || (inSignDomain == sdNegative && current < 0) || (inSignDomain == sdPositive && current > 0))
    {
      if (current < range.lower || !haveLower)
      {
        range.lower = current;
        haveLower = true;
      }
      if (current > range.upper || !haveUpper)
      {
        range.upper = current;
        haveUpper = true;
      }
    }
    ++it;
  }

  foundRange = true;
  return range;
}

QCPStatisticalBox::QCPStatisticalBox(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPAbstractPlottable(keyAxis, valueAxis),
  mKey(0),
  mMinimum(0),
  mLowerQuartile(0),
  mMedian(0),
  mUpperQuartile(0),
  mMaximum(0)
{
  setOutlierStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, 6));
  setWhiskerWidth(0.2);
  setWidth(0.5);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue, 2.5));
  setMedianPen(QPen(Qt::black, 3, Qt::SolidLine, Qt::FlatCap));
  setWhiskerPen(QPen(Qt::black, 0, Qt::DashLine, Qt::FlatCap));
  setWhiskerBarPen(QPen(Qt::black));
  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
}

void QCPStatisticalBox::setKey(double key)
{
  mKey = key;
}

void QCPStatisticalBox::setMinimum(double value)
{
  mMinimum = value;
}

void QCPStatisticalBox::setLowerQuartile(double value)
{
  mLowerQuartile = value;
}

void QCPStatisticalBox::setMedian(double value)
{
  mMedian = value;
}

void QCPStatisticalBox::setUpperQuartile(double value)
{
  mUpperQuartile = value;
}

void QCPStatisticalBox::setMaximum(double value)
{
  mMaximum = value;
}

void QCPStatisticalBox::setOutliers(const QVector<double> &values)
{
  mOutliers = values;
}

void QCPStatisticalBox::setData(double key, double minimum, double lowerQuartile, double median, double upperQuartile, double maximum)
{
  setKey(key);
  setMinimum(minimum);
  setLowerQuartile(lowerQuartile);
  setMedian(median);
  setUpperQuartile(upperQuartile);
  setMaximum(maximum);
}

void QCPStatisticalBox::setWidth(double width)
{
  mWidth = width;
}

void QCPStatisticalBox::setWhiskerWidth(double width)
{
  mWhiskerWidth = width;
}

void QCPStatisticalBox::setWhiskerPen(const QPen &pen)
{
  mWhiskerPen = pen;
}

void QCPStatisticalBox::setWhiskerBarPen(const QPen &pen)
{
  mWhiskerBarPen = pen;
}

void QCPStatisticalBox::setMedianPen(const QPen &pen)
{
  mMedianPen = pen;
}

void QCPStatisticalBox::setOutlierStyle(const QCPScatterStyle &style)
{
  mOutlierStyle = style;
}

void QCPStatisticalBox::clearData()
{
  setOutliers(QVector<double>());
  setKey(0);
  setMinimum(0);
  setLowerQuartile(0);
  setMedian(0);
  setUpperQuartile(0);
  setMaximum(0);
}

double QCPStatisticalBox::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return -1; }

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()))
  {
    double posKey, posValue;
    pixelsToCoords(pos, posKey, posValue);

    QCPRange keyRange(mKey-mWidth*0.5, mKey+mWidth*0.5);
    QCPRange valueRange(mLowerQuartile, mUpperQuartile);
    if (keyRange.contains(posKey) && valueRange.contains(posValue))
      return mParentPlot->selectionTolerance()*0.99;

    if (QCPRange(mMinimum, mMaximum).contains(posValue))
      return qAbs(mKeyAxis.data()->coordToPixel(mKey)-mKeyAxis.data()->coordToPixel(posKey));
  }
  return -1;
}

void QCPStatisticalBox::draw(QCPPainter *painter)
{
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }

#ifdef QCUSTOMPLOT_CHECK_DATA
  if (QCP::isInvalidData(mKey, mMedian) ||
      QCP::isInvalidData(mLowerQuartile, mUpperQuartile) ||
      QCP::isInvalidData(mMinimum, mMaximum))
    qDebug() << Q_FUNC_INFO << "Data point at" << mKey << "of drawn range has invalid data." << "Plottable name:" << name();
  for (int i=0; i<mOutliers.size(); ++i)
    if (QCP::isInvalidData(mOutliers.at(i)))
      qDebug() << Q_FUNC_INFO << "Data point outlier at" << mKey << "of drawn range invalid." << "Plottable name:" << name();
#endif

  QRectF quartileBox;
  drawQuartileBox(painter, &quartileBox);

  painter->save();
  painter->setClipRect(quartileBox, Qt::IntersectClip);
  drawMedian(painter);
  painter->restore();

  drawWhiskers(painter);
  drawOutliers(painter);
}

void QCPStatisticalBox::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{

  applyDefaultAntialiasingHint(painter);
  painter->setPen(mPen);
  painter->setBrush(mBrush);
  QRectF r = QRectF(0, 0, rect.width()*0.67, rect.height()*0.67);
  r.moveCenter(rect.center());
  painter->drawRect(r);
}

void QCPStatisticalBox::drawQuartileBox(QCPPainter *painter, QRectF *quartileBox) const
{
  QRectF box;
  box.setTopLeft(coordsToPixels(mKey-mWidth*0.5, mUpperQuartile));
  box.setBottomRight(coordsToPixels(mKey+mWidth*0.5, mLowerQuartile));
  applyDefaultAntialiasingHint(painter);
  painter->setPen(mainPen());
  painter->setBrush(mainBrush());
  painter->drawRect(box);
  if (quartileBox)
    *quartileBox = box;
}

void QCPStatisticalBox::drawMedian(QCPPainter *painter) const
{
  QLineF medianLine;
  medianLine.setP1(coordsToPixels(mKey-mWidth*0.5, mMedian));
  medianLine.setP2(coordsToPixels(mKey+mWidth*0.5, mMedian));
  applyDefaultAntialiasingHint(painter);
  painter->setPen(mMedianPen);
  painter->drawLine(medianLine);
}

void QCPStatisticalBox::drawWhiskers(QCPPainter *painter) const
{
  QLineF backboneMin, backboneMax, barMin, barMax;
  backboneMax.setPoints(coordsToPixels(mKey, mUpperQuartile), coordsToPixels(mKey, mMaximum));
  backboneMin.setPoints(coordsToPixels(mKey, mLowerQuartile), coordsToPixels(mKey, mMinimum));
  barMax.setPoints(coordsToPixels(mKey-mWhiskerWidth*0.5, mMaximum), coordsToPixels(mKey+mWhiskerWidth*0.5, mMaximum));
  barMin.setPoints(coordsToPixels(mKey-mWhiskerWidth*0.5, mMinimum), coordsToPixels(mKey+mWhiskerWidth*0.5, mMinimum));
  applyErrorBarsAntialiasingHint(painter);
  painter->setPen(mWhiskerPen);
  painter->drawLine(backboneMin);
  painter->drawLine(backboneMax);
  painter->setPen(mWhiskerBarPen);
  painter->drawLine(barMin);
  painter->drawLine(barMax);
}

void QCPStatisticalBox::drawOutliers(QCPPainter *painter) const
{
  applyScattersAntialiasingHint(painter);
  mOutlierStyle.applyTo(painter, mPen);
  for (int i=0; i<mOutliers.size(); ++i)
    mOutlierStyle.drawShape(painter, coordsToPixels(mKey, mOutliers.at(i)));
}

QCPRange QCPStatisticalBox::getKeyRange(bool &foundRange, SignDomain inSignDomain) const
{
  foundRange = true;
  if (inSignDomain == sdBoth)
  {
    return QCPRange(mKey-mWidth*0.5, mKey+mWidth*0.5);
  } else if (inSignDomain == sdNegative)
  {
    if (mKey+mWidth*0.5 < 0)
      return QCPRange(mKey-mWidth*0.5, mKey+mWidth*0.5);
    else if (mKey < 0)
      return QCPRange(mKey-mWidth*0.5, mKey);
    else
    {
      foundRange = false;
      return QCPRange();
    }
  } else if (inSignDomain == sdPositive)
  {
    if (mKey-mWidth*0.5 > 0)
      return QCPRange(mKey-mWidth*0.5, mKey+mWidth*0.5);
    else if (mKey > 0)
      return QCPRange(mKey, mKey+mWidth*0.5);
    else
    {
      foundRange = false;
      return QCPRange();
    }
  }
  foundRange = false;
  return QCPRange();
}

QCPRange QCPStatisticalBox::getValueRange(bool &foundRange, SignDomain inSignDomain) const
{
  QVector<double> values;
  values.reserve(mOutliers.size() + 5);
  values << mMaximum << mUpperQuartile << mMedian << mLowerQuartile << mMinimum;
  values << mOutliers;

  bool haveUpper = false;
  bool haveLower = false;
  double upper = 0;
  double lower = 0;
  for (int i=0; i<values.size(); ++i)
  {
    if ((inSignDomain == sdNegative && values.at(i) < 0) ||
        (inSignDomain == sdPositive && values.at(i) > 0) ||
        (inSignDomain == sdBoth))
    {
      if (values.at(i) > upper || !haveUpper)
      {
        upper = values.at(i);
        haveUpper = true;
      }
      if (values.at(i) < lower || !haveLower)
      {
        lower = values.at(i);
        haveLower = true;
      }
    }
  }

  if (haveLower && haveUpper)
  {
    foundRange = true;
    return QCPRange(lower, upper);
  } else
  {
    foundRange = false;
    return QCPRange();
  }
}

QCPColorMapData::QCPColorMapData(int keySize, int valueSize, const QCPRange &keyRange, const QCPRange &valueRange) :
  mKeySize(0),
  mValueSize(0),
  mKeyRange(keyRange),
  mValueRange(valueRange),
  mIsEmpty(true),
  mData(0),
  mDataModified(true)
{
  setSize(keySize, valueSize);
  fill(0);
}

QCPColorMapData::~QCPColorMapData()
{
  if (mData)
    delete[] mData;
}

QCPColorMapData::QCPColorMapData(const QCPColorMapData &other) :
  mKeySize(0),
  mValueSize(0),
  mIsEmpty(true),
  mData(0),
  mDataModified(true)
{
  *this = other;
}

QCPColorMapData &QCPColorMapData::operator=(const QCPColorMapData &other)
{
  if (&other != this)
  {
    const int keySize = other.keySize();
    const int valueSize = other.valueSize();
    setSize(keySize, valueSize);
    setRange(other.keyRange(), other.valueRange());
    if (!mIsEmpty)
      memcpy(mData, other.mData, sizeof(mData[0])*keySize*valueSize);
    mDataBounds = other.mDataBounds;
    mDataModified = true;
  }
  return *this;
}

double QCPColorMapData::data(double key, double value)
{
  int keyCell = (key-mKeyRange.lower)/(mKeyRange.upper-mKeyRange.lower)*(mKeySize-1)+0.5;
  int valueCell = (1.0-(value-mValueRange.lower)/(mValueRange.upper-mValueRange.lower))*(mValueSize-1)+0.5;
  if (keyCell >= 0 && keyCell < mKeySize && valueCell >= 0 && valueCell < mValueSize)
    return mData[valueCell*mKeySize + keyCell];
  else
    return 0;
}

double QCPColorMapData::cell(int keyIndex, int valueIndex)
{
  if (keyIndex >= 0 && keyIndex < mKeySize && valueIndex >= 0 && valueIndex < mValueSize)
    return mData[valueIndex*mKeySize + keyIndex];
  else
    return 0;
}

void QCPColorMapData::setSize(int keySize, int valueSize)
{
  if (keySize != mKeySize || valueSize != mValueSize)
  {
    mKeySize = keySize;
    mValueSize = valueSize;
    if (mData)
      delete[] mData;
    mIsEmpty = mKeySize == 0 || mValueSize == 0;
    if (!mIsEmpty)
    {
#ifdef __EXCEPTIONS
      try {
#endif
      mData = new double[mKeySize*mValueSize];
#ifdef __EXCEPTIONS
      } catch (...) { mData = 0; }
#endif
      if (mData)
        fill(0);
      else
        qDebug() << Q_FUNC_INFO << "out of memory for data dimensions "<< mKeySize << "*" << mValueSize;
    } else
      mData = 0;
    mDataModified = true;
  }
}

void QCPColorMapData::setKeySize(int keySize)
{
  setSize(keySize, mValueSize);
}

void QCPColorMapData::setValueSize(int valueSize)
{
  setSize(mKeySize, valueSize);
}

void QCPColorMapData::setRange(const QCPRange &keyRange, const QCPRange &valueRange)
{
  setKeyRange(keyRange);
  setValueRange(valueRange);
}

void QCPColorMapData::setKeyRange(const QCPRange &keyRange)
{
  mKeyRange = keyRange;
}

void QCPColorMapData::setValueRange(const QCPRange &valueRange)
{
  mValueRange = valueRange;
}

void QCPColorMapData::setData(double key, double value, double z)
{
  int keyCell = (key-mKeyRange.lower)/(mKeyRange.upper-mKeyRange.lower)*(mKeySize-1)+0.5;
  int valueCell = (value-mValueRange.lower)/(mValueRange.upper-mValueRange.lower)*(mValueSize-1)+0.5;
  if (keyCell >= 0 && keyCell < mKeySize && valueCell >= 0 && valueCell < mValueSize)
  {
    mData[valueCell*mKeySize + keyCell] = z;
    if (z < mDataBounds.lower)
      mDataBounds.lower = z;
    if (z > mDataBounds.upper)
      mDataBounds.upper = z;
     mDataModified = true;
  }
}

void QCPColorMapData::setCell(int keyIndex, int valueIndex, double z)
{
  if (keyIndex >= 0 && keyIndex < mKeySize && valueIndex >= 0 && valueIndex < mValueSize)
  {
    mData[valueIndex*mKeySize + keyIndex] = z;
    if (z < mDataBounds.lower)
      mDataBounds.lower = z;
    if (z > mDataBounds.upper)
      mDataBounds.upper = z;
     mDataModified = true;
  }
}

void QCPColorMapData::recalculateDataBounds()
{
  if (mKeySize > 0 && mValueSize > 0)
  {
    double minHeight = mData[0];
    double maxHeight = mData[0];
    const int dataCount = mValueSize*mKeySize;
    for (int i=0; i<dataCount; ++i)
    {
      if (mData[i] > maxHeight)
        maxHeight = mData[i];
      if (mData[i] < minHeight)
        minHeight = mData[i];
    }
    mDataBounds.lower = minHeight;
    mDataBounds.upper = maxHeight;
  }
}

void QCPColorMapData::clear()
{
  setSize(0, 0);
}

void QCPColorMapData::fill(double z)
{
  const int dataCount = mValueSize*mKeySize;
  for (int i=0; i<dataCount; ++i)
    mData[i] = z;
  mDataBounds = QCPRange(z, z);
}

void QCPColorMapData::coordToCell(double key, double value, int *keyIndex, int *valueIndex) const
{
  if (keyIndex)
    *keyIndex = (key-mKeyRange.lower)/(mKeyRange.upper-mKeyRange.lower)*(mKeySize-1)+0.5;
  if (valueIndex)
    *valueIndex = (value-mValueRange.lower)/(mValueRange.upper-mValueRange.lower)*(mValueSize-1)+0.5;
}

void QCPColorMapData::cellToCoord(int keyIndex, int valueIndex, double *key, double *value) const
{
  if (key)
    *key = keyIndex/(double)(mKeySize-1)*(mKeyRange.upper-mKeyRange.lower)+mKeyRange.lower;
  if (value)
    *value = valueIndex/(double)(mValueSize-1)*(mValueRange.upper-mValueRange.lower)+mValueRange.lower;
}

QCPColorMap::QCPColorMap(QCPAxis *keyAxis, QCPAxis *valueAxis) :
  QCPAbstractPlottable(keyAxis, valueAxis),
  mDataScaleType(QCPAxis::stLinear),
  mMapData(new QCPColorMapData(10, 10, QCPRange(0, 5), QCPRange(0, 5))),
  mInterpolate(true),
  mTightBoundary(false),
  mMapImageInvalidated(true)
{
}

QCPColorMap::~QCPColorMap()
{
  delete mMapData;
}

void QCPColorMap::setData(QCPColorMapData *data, bool copy)
{
  if (copy)
  {
    *mMapData = *data;
  } else
  {
    delete mMapData;
    mMapData = data;
  }
  mMapImageInvalidated = true;
}

void QCPColorMap::setDataRange(const QCPRange &dataRange)
{
  if (!QCPRange::validRange(dataRange)) return;
  if (mDataRange.lower != dataRange.lower || mDataRange.upper != dataRange.upper)
  {
    if (mDataScaleType == QCPAxis::stLogarithmic)
      mDataRange = dataRange.sanitizedForLogScale();
    else
      mDataRange = dataRange.sanitizedForLinScale();
    mMapImageInvalidated = true;
    Q_EMIT dataRangeChanged(mDataRange);
  }
}

void QCPColorMap::setDataScaleType(QCPAxis::ScaleType scaleType)
{
  if (mDataScaleType != scaleType)
  {
    mDataScaleType = scaleType;
    mMapImageInvalidated = true;
    Q_EMIT dataScaleTypeChanged(mDataScaleType);
    if (mDataScaleType == QCPAxis::stLogarithmic)
      setDataRange(mDataRange.sanitizedForLogScale());
  }
}

void QCPColorMap::setGradient(const QCPColorGradient &gradient)
{
  if (mGradient != gradient)
  {
    mGradient = gradient;
    mMapImageInvalidated = true;
    Q_EMIT gradientChanged(mGradient);
  }
}

void QCPColorMap::setInterpolate(bool enabled)
{
  mInterpolate = enabled;
}

void QCPColorMap::setTightBoundary(bool enabled)
{
  mTightBoundary = enabled;
}

void QCPColorMap::setColorScale(QCPColorScale *colorScale)
{
  if (mColorScale)
  {
    disconnect(this, SIGNAL(dataRangeChanged(QCPRange)), mColorScale.data(), SLOT(setDataRange(QCPRange)));
    disconnect(this, SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), mColorScale.data(), SLOT(setDataScaleType(QCPAxis::ScaleType)));
    disconnect(this, SIGNAL(gradientChanged(QCPColorGradient)), mColorScale.data(), SLOT(setGradient(QCPColorGradient)));
    disconnect(mColorScale.data(), SIGNAL(dataRangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
    disconnect(mColorScale.data(), SIGNAL(gradientChanged(QCPColorGradient)), this, SLOT(setGradient(QCPColorGradient)));
    disconnect(mColorScale.data(), SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
  }
  mColorScale = colorScale;
  if (mColorScale)
  {
    setGradient(mColorScale.data()->gradient());
    setDataRange(mColorScale.data()->dataRange());
    setDataScaleType(mColorScale.data()->dataScaleType());
    connect(this, SIGNAL(dataRangeChanged(QCPRange)), mColorScale.data(), SLOT(setDataRange(QCPRange)));
    connect(this, SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), mColorScale.data(), SLOT(setDataScaleType(QCPAxis::ScaleType)));
    connect(this, SIGNAL(gradientChanged(QCPColorGradient)), mColorScale.data(), SLOT(setGradient(QCPColorGradient)));
    connect(mColorScale.data(), SIGNAL(dataRangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
    connect(mColorScale.data(), SIGNAL(gradientChanged(QCPColorGradient)), this, SLOT(setGradient(QCPColorGradient)));
    connect(mColorScale.data(), SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
  }
}

void QCPColorMap::rescaleDataRange(bool recalculateDataBounds)
{
  if (recalculateDataBounds)
    mMapData->recalculateDataBounds();
  setDataRange(mMapData->dataBounds());
}

void QCPColorMap::updateLegendIcon(Qt::TransformationMode transformMode, const QSize &thumbSize)
{
  if (mMapImage.isNull() && !data()->isEmpty())
    updateMapImage();

  if (!mMapImage.isNull())
  {
    bool mirrorX = (keyAxis()->orientation() == Qt::Horizontal ? keyAxis() : valueAxis())->rangeReversed();
    bool mirrorY = (valueAxis()->orientation() == Qt::Vertical ? valueAxis() : keyAxis())->rangeReversed();
    mLegendIcon = QPixmap::fromImage(mMapImage.mirrored(mirrorX, mirrorY)).scaled(thumbSize, Qt::KeepAspectRatio, transformMode);
  }
}

void QCPColorMap::clearData()
{
  mMapData->clear();
}

double QCPColorMap::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;
  if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return -1; }

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()))
  {
    double posKey, posValue;
    pixelsToCoords(pos, posKey, posValue);
    if (mMapData->keyRange().contains(posKey) && mMapData->valueRange().contains(posValue))
      return mParentPlot->selectionTolerance()*0.99;
  }
  return -1;
}

void QCPColorMap::updateMapImage()
{
  QCPAxis *keyAxis = mKeyAxis.data();
  if (!keyAxis) return;

  if (keyAxis->orientation() == Qt::Horizontal && (mMapImage.size().width() != mMapData->keySize() || mMapImage.size().height() != mMapData->valueSize()))
    mMapImage = QImage(QSize(mMapData->keySize(), mMapData->valueSize()), QImage::Format_RGB32);
  else if (keyAxis->orientation() == Qt::Vertical && (mMapImage.size().width() != mMapData->valueSize() || mMapImage.size().height() != mMapData->keySize()))
    mMapImage = QImage(QSize(mMapData->valueSize(), mMapData->keySize()), QImage::Format_RGB32);

  const int keySize = mMapData->keySize();
  const int valueSize = mMapData->valueSize();
  const double *rawData = mMapData->mData;

  if (keyAxis->orientation() == Qt::Horizontal)
  {
    const int lineCount = valueSize;
    const int rowCount = keySize;
    for (int line=0; line<lineCount; ++line)
    {
      QRgb* pixels = reinterpret_cast<QRgb*>(mMapImage.scanLine(lineCount-1-line));
      mGradient.colorize(rawData+line*rowCount, mDataRange, pixels, rowCount, 1, mDataScaleType==QCPAxis::stLogarithmic);
    }
  } else
  {
    const int lineCount = keySize;
    const int rowCount = valueSize;
    for (int line=0; line<lineCount; ++line)
    {
      QRgb* pixels = reinterpret_cast<QRgb*>(mMapImage.scanLine(lineCount-1-line));
      mGradient.colorize(rawData+line, mDataRange, pixels, rowCount, lineCount, mDataScaleType==QCPAxis::stLogarithmic);
    }
  }

  mMapData->mDataModified = false;
  mMapImageInvalidated = false;
}

void QCPColorMap::draw(QCPPainter *painter)
{
  if (mMapData->isEmpty()) return;
  if (!mKeyAxis || !mValueAxis) return;
  applyDefaultAntialiasingHint(painter);

  if (mMapData->mDataModified || mMapImageInvalidated)
    updateMapImage();

  double halfSampleKey = 0;
  double halfSampleValue = 0;
  if (mMapData->keySize() > 1)
    halfSampleKey = 0.5*mMapData->keyRange().size()/(double)(mMapData->keySize()-1);
  if (mMapData->valueSize() > 1)
    halfSampleValue = 0.5*mMapData->valueRange().size()/(double)(mMapData->valueSize()-1);
  QRectF imageRect(coordsToPixels(mMapData->keyRange().lower-halfSampleKey, mMapData->valueRange().lower-halfSampleValue),
                   coordsToPixels(mMapData->keyRange().upper+halfSampleKey, mMapData->valueRange().upper+halfSampleValue));
  imageRect = imageRect.normalized();
  bool mirrorX = (keyAxis()->orientation() == Qt::Horizontal ? keyAxis() : valueAxis())->rangeReversed();
  bool mirrorY = (valueAxis()->orientation() == Qt::Vertical ? valueAxis() : keyAxis())->rangeReversed();
  bool smoothBackup = painter->renderHints().testFlag(QPainter::SmoothPixmapTransform);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, mInterpolate);
  QRegion clipBackup;
  if (mTightBoundary)
  {
    clipBackup = painter->clipRegion();
    painter->setClipRect(QRectF(coordsToPixels(mMapData->keyRange().lower, mMapData->valueRange().lower),
                                coordsToPixels(mMapData->keyRange().upper, mMapData->valueRange().upper)).normalized(), Qt::IntersectClip);
  }
  painter->drawImage(imageRect, mMapImage.mirrored(mirrorX, mirrorY));
  if (mTightBoundary)
    painter->setClipRegion(clipBackup);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, smoothBackup);
}

void QCPColorMap::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{
  applyDefaultAntialiasingHint(painter);

  if (!mLegendIcon.isNull())
  {
    QPixmap scaledIcon = mLegendIcon.scaled(rect.size().toSize(), Qt::KeepAspectRatio, Qt::FastTransformation);
    QRectF iconRect = QRectF(0, 0, scaledIcon.width(), scaledIcon.height());
    iconRect.moveCenter(rect.center());
    painter->drawPixmap(iconRect.topLeft(), scaledIcon);
  }

}

QCPRange QCPColorMap::getKeyRange(bool &foundRange, SignDomain inSignDomain) const
{
  foundRange = true;
  QCPRange result = mMapData->keyRange();
  result.normalize();
  if (inSignDomain == QCPAbstractPlottable::sdPositive)
  {
    if (result.lower <= 0 && result.upper > 0)
      result.lower = result.upper*1e-3;
    else if (result.lower <= 0 && result.upper <= 0)
      foundRange = false;
  } else if (inSignDomain == QCPAbstractPlottable::sdNegative)
  {
    if (result.upper >= 0 && result.lower < 0)
      result.upper = result.lower*1e-3;
    else if (result.upper >= 0 && result.lower >= 0)
      foundRange = false;
  }
  return result;
}

QCPRange QCPColorMap::getValueRange(bool &foundRange, SignDomain inSignDomain) const
{
  foundRange = true;
  QCPRange result = mMapData->valueRange();
  result.normalize();
  if (inSignDomain == QCPAbstractPlottable::sdPositive)
  {
    if (result.lower <= 0 && result.upper > 0)
      result.lower = result.upper*1e-3;
    else if (result.lower <= 0 && result.upper <= 0)
      foundRange = false;
  } else if (inSignDomain == QCPAbstractPlottable::sdNegative)
  {
    if (result.upper >= 0 && result.lower < 0)
      result.upper = result.lower*1e-3;
    else if (result.upper >= 0 && result.lower >= 0)
      foundRange = false;
  }
  return result;
}

QCPItemStraightLine::QCPItemStraightLine(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  point1(createPosition("point1")),
  point2(createPosition("point2"))
{
  point1->setCoords(0, 0);
  point2->setCoords(1, 1);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue,2));
}

QCPItemStraightLine::~QCPItemStraightLine()
{
}

void QCPItemStraightLine::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemStraightLine::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

double QCPItemStraightLine::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  return distToStraightLine(QVector2D(point1->pixelPoint()), QVector2D(point2->pixelPoint()-point1->pixelPoint()), QVector2D(pos));
}

void QCPItemStraightLine::draw(QCPPainter *painter)
{
  QVector2D start(point1->pixelPoint());
  QVector2D end(point2->pixelPoint());

  double clipPad = mainPen().widthF();
  QLineF line = getRectClippedStraightLine(start, end-start, clipRect().adjusted(-clipPad, -clipPad, clipPad, clipPad));

  if (!line.isNull())
  {
    painter->setPen(mainPen());
    painter->drawLine(line);
  }
}

double QCPItemStraightLine::distToStraightLine(const QVector2D &base, const QVector2D &vec, const QVector2D &point) const
{
  return qAbs((base.y()-point.y())*vec.x()-(base.x()-point.x())*vec.y())/vec.length();
}

QLineF QCPItemStraightLine::getRectClippedStraightLine(const QVector2D &base, const QVector2D &vec, const QRect &rect) const
{
  double bx, by;
  double gamma;
  QLineF result;
  if (vec.x() == 0 && vec.y() == 0)
    return result;
  if (qFuzzyIsNull(vec.x()))
  {

    bx = rect.left();
    by = rect.top();
    gamma = base.x()-bx + (by-base.y())*vec.x()/vec.y();
    if (gamma >= 0 && gamma <= rect.width())
      result.setLine(bx+gamma, rect.top(), bx+gamma, rect.bottom());
  } else if (qFuzzyIsNull(vec.y()))
  {

    bx = rect.left();
    by = rect.top();
    gamma = base.y()-by + (bx-base.x())*vec.y()/vec.x();
    if (gamma >= 0 && gamma <= rect.height())
      result.setLine(rect.left(), by+gamma, rect.right(), by+gamma);
  } else
  {
    QList<QVector2D> pointVectors;

    bx = rect.left();
    by = rect.top();
    gamma = base.x()-bx + (by-base.y())*vec.x()/vec.y();
    if (gamma >= 0 && gamma <= rect.width())
      pointVectors.append(QVector2D(bx+gamma, by));

    bx = rect.left();
    by = rect.bottom();
    gamma = base.x()-bx + (by-base.y())*vec.x()/vec.y();
    if (gamma >= 0 && gamma <= rect.width())
      pointVectors.append(QVector2D(bx+gamma, by));

    bx = rect.left();
    by = rect.top();
    gamma = base.y()-by + (bx-base.x())*vec.y()/vec.x();
    if (gamma >= 0 && gamma <= rect.height())
      pointVectors.append(QVector2D(bx, by+gamma));

    bx = rect.right();
    by = rect.top();
    gamma = base.y()-by + (bx-base.x())*vec.y()/vec.x();
    if (gamma >= 0 && gamma <= rect.height())
      pointVectors.append(QVector2D(bx, by+gamma));

    if (pointVectors.size() == 2)
    {
      result.setPoints(pointVectors.at(0).toPointF(), pointVectors.at(1).toPointF());
    } else if (pointVectors.size() > 2)
    {

      double distSqrMax = 0;
      QVector2D pv1, pv2;
      for (int i=0; i<pointVectors.size()-1; ++i)
      {
        for (int k=i+1; k<pointVectors.size(); ++k)
        {
          double distSqr = (pointVectors.at(i)-pointVectors.at(k)).lengthSquared();
          if (distSqr > distSqrMax)
          {
            pv1 = pointVectors.at(i);
            pv2 = pointVectors.at(k);
            distSqrMax = distSqr;
          }
        }
      }
      result.setPoints(pv1.toPointF(), pv2.toPointF());
    }
  }
  return result;
}

QPen QCPItemStraightLine::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QCPItemLine::QCPItemLine(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  start(createPosition("start")),
  end(createPosition("end"))
{
  start->setCoords(0, 0);
  end->setCoords(1, 1);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue,2));
}

QCPItemLine::~QCPItemLine()
{
}

void QCPItemLine::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemLine::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemLine::setHead(const QCPLineEnding &head)
{
  mHead = head;
}

void QCPItemLine::setTail(const QCPLineEnding &tail)
{
  mTail = tail;
}

double QCPItemLine::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  return qSqrt(distSqrToLine(start->pixelPoint(), end->pixelPoint(), pos));
}

void QCPItemLine::draw(QCPPainter *painter)
{
  QVector2D startVec(start->pixelPoint());
  QVector2D endVec(end->pixelPoint());
  if (startVec.toPoint() == endVec.toPoint())
    return;

  double clipPad = qMax(mHead.boundingDistance(), mTail.boundingDistance());
  clipPad = qMax(clipPad, (double)mainPen().widthF());
  QLineF line = getRectClippedLine(startVec, endVec, clipRect().adjusted(-clipPad, -clipPad, clipPad, clipPad));

  if (!line.isNull())
  {
    painter->setPen(mainPen());
    painter->drawLine(line);
    painter->setBrush(Qt::SolidPattern);
    if (mTail.style() != QCPLineEnding::esNone)
      mTail.draw(painter, startVec, startVec-endVec);
    if (mHead.style() != QCPLineEnding::esNone)
      mHead.draw(painter, endVec, endVec-startVec);
  }
}

QLineF QCPItemLine::getRectClippedLine(const QVector2D &start, const QVector2D &end, const QRect &rect) const
{
  bool containsStart = rect.contains(start.x(), start.y());
  bool containsEnd = rect.contains(end.x(), end.y());
  if (containsStart && containsEnd)
    return QLineF(start.toPointF(), end.toPointF());

  QVector2D base = start;
  QVector2D vec = end-start;
  double bx, by;
  double gamma, mu;
  QLineF result;
  QList<QVector2D> pointVectors;

  if (!qFuzzyIsNull(vec.y()))
  {

    bx = rect.left();
    by = rect.top();
    mu = (by-base.y())/vec.y();
    if (mu >= 0 && mu <= 1)
    {
      gamma = base.x()-bx + mu*vec.x();
      if (gamma >= 0 && gamma <= rect.width())
        pointVectors.append(QVector2D(bx+gamma, by));
    }

    bx = rect.left();
    by = rect.bottom();
    mu = (by-base.y())/vec.y();
    if (mu >= 0 && mu <= 1)
    {
      gamma = base.x()-bx + mu*vec.x();
      if (gamma >= 0 && gamma <= rect.width())
        pointVectors.append(QVector2D(bx+gamma, by));
    }
  }
  if (!qFuzzyIsNull(vec.x()))
  {

    bx = rect.left();
    by = rect.top();
    mu = (bx-base.x())/vec.x();
    if (mu >= 0 && mu <= 1)
    {
      gamma = base.y()-by + mu*vec.y();
      if (gamma >= 0 && gamma <= rect.height())
        pointVectors.append(QVector2D(bx, by+gamma));
    }

    bx = rect.right();
    by = rect.top();
    mu = (bx-base.x())/vec.x();
    if (mu >= 0 && mu <= 1)
    {
      gamma = base.y()-by + mu*vec.y();
      if (gamma >= 0 && gamma <= rect.height())
        pointVectors.append(QVector2D(bx, by+gamma));
    }
  }

  if (containsStart)
    pointVectors.append(start);
  if (containsEnd)
    pointVectors.append(end);

  if (pointVectors.size() == 2)
  {
    result.setPoints(pointVectors.at(0).toPointF(), pointVectors.at(1).toPointF());
  } else if (pointVectors.size() > 2)
  {

    double distSqrMax = 0;
    QVector2D pv1, pv2;
    for (int i=0; i<pointVectors.size()-1; ++i)
    {
      for (int k=i+1; k<pointVectors.size(); ++k)
      {
        double distSqr = (pointVectors.at(i)-pointVectors.at(k)).lengthSquared();
        if (distSqr > distSqrMax)
        {
          pv1 = pointVectors.at(i);
          pv2 = pointVectors.at(k);
          distSqrMax = distSqr;
        }
      }
    }
    result.setPoints(pv1.toPointF(), pv2.toPointF());
  }
  return result;
}

QPen QCPItemLine::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QCPItemCurve::QCPItemCurve(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  start(createPosition("start")),
  startDir(createPosition("startDir")),
  endDir(createPosition("endDir")),
  end(createPosition("end"))
{
  start->setCoords(0, 0);
  startDir->setCoords(0.5, 0);
  endDir->setCoords(0, 0.5);
  end->setCoords(1, 1);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue,2));
}

QCPItemCurve::~QCPItemCurve()
{
}

void QCPItemCurve::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemCurve::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemCurve::setHead(const QCPLineEnding &head)
{
  mHead = head;
}

void QCPItemCurve::setTail(const QCPLineEnding &tail)
{
  mTail = tail;
}

double QCPItemCurve::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QPointF startVec(start->pixelPoint());
  QPointF startDirVec(startDir->pixelPoint());
  QPointF endDirVec(endDir->pixelPoint());
  QPointF endVec(end->pixelPoint());

  QPainterPath cubicPath(startVec);
  cubicPath.cubicTo(startDirVec, endDirVec, endVec);

  QPolygonF polygon = cubicPath.toSubpathPolygons().first();
  double minDistSqr = std::numeric_limits<double>::max();
  for (int i=1; i<polygon.size(); ++i)
  {
    double distSqr = distSqrToLine(polygon.at(i-1), polygon.at(i), pos);
    if (distSqr < minDistSqr)
      minDistSqr = distSqr;
  }
  return qSqrt(minDistSqr);
}

void QCPItemCurve::draw(QCPPainter *painter)
{
  QPointF startVec(start->pixelPoint());
  QPointF startDirVec(startDir->pixelPoint());
  QPointF endDirVec(endDir->pixelPoint());
  QPointF endVec(end->pixelPoint());
  if (QVector2D(endVec-startVec).length() > 1e10f)
    return;

  QPainterPath cubicPath(startVec);
  cubicPath.cubicTo(startDirVec, endDirVec, endVec);

  QRect clip = clipRect().adjusted(-mainPen().widthF(), -mainPen().widthF(), mainPen().widthF(), mainPen().widthF());
  QRect cubicRect = cubicPath.controlPointRect().toRect();
  if (cubicRect.isEmpty())
    cubicRect.adjust(0, 0, 1, 1);
  if (clip.intersects(cubicRect))
  {
    painter->setPen(mainPen());
    painter->drawPath(cubicPath);
    painter->setBrush(Qt::SolidPattern);
    if (mTail.style() != QCPLineEnding::esNone)
      mTail.draw(painter, QVector2D(startVec), M_PI-cubicPath.angleAtPercent(0)/180.0*M_PI);
    if (mHead.style() != QCPLineEnding::esNone)
      mHead.draw(painter, QVector2D(endVec), -cubicPath.angleAtPercent(1)/180.0*M_PI);
  }
}

QPen QCPItemCurve::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QCPItemRect::QCPItemRect(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  topLeft(createPosition("topLeft")),
  bottomRight(createPosition("bottomRight")),
  top(createAnchor("top", aiTop)),
  topRight(createAnchor("topRight", aiTopRight)),
  right(createAnchor("right", aiRight)),
  bottom(createAnchor("bottom", aiBottom)),
  bottomLeft(createAnchor("bottomLeft", aiBottomLeft)),
  left(createAnchor("left", aiLeft))
{
  topLeft->setCoords(0, 1);
  bottomRight->setCoords(1, 0);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue,2));
  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
}

QCPItemRect::~QCPItemRect()
{
}

void QCPItemRect::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemRect::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemRect::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPItemRect::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

double QCPItemRect::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QRectF rect = QRectF(topLeft->pixelPoint(), bottomRight->pixelPoint()).normalized();
  bool filledRect = mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0;
  return rectSelectTest(rect, pos, filledRect);
}

void QCPItemRect::draw(QCPPainter *painter)
{
  QPointF p1 = topLeft->pixelPoint();
  QPointF p2 = bottomRight->pixelPoint();
  if (p1.toPoint() == p2.toPoint())
    return;
  QRectF rect = QRectF(p1, p2).normalized();
  double clipPad = mainPen().widthF();
  QRectF boundingRect = rect.adjusted(-clipPad, -clipPad, clipPad, clipPad);
  if (boundingRect.intersects(clipRect()))
  {
    painter->setPen(mainPen());
    painter->setBrush(mainBrush());
    painter->drawRect(rect);
  }
}

QPointF QCPItemRect::anchorPixelPoint(int anchorId) const
{
  QRectF rect = QRectF(topLeft->pixelPoint(), bottomRight->pixelPoint());
  switch (anchorId)
  {
    case aiTop:         return (rect.topLeft()+rect.topRight())*0.5;
    case aiTopRight:    return rect.topRight();
    case aiRight:       return (rect.topRight()+rect.bottomRight())*0.5;
    case aiBottom:      return (rect.bottomLeft()+rect.bottomRight())*0.5;
    case aiBottomLeft:  return rect.bottomLeft();
    case aiLeft:        return (rect.topLeft()+rect.bottomLeft())*0.5;
  }

  qDebug() << Q_FUNC_INFO << "invalid anchorId" << anchorId;
  return QPointF();
}

QPen QCPItemRect::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPItemRect::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}

QCPItemText::QCPItemText(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  position(createPosition("position")),
  topLeft(createAnchor("topLeft", aiTopLeft)),
  top(createAnchor("top", aiTop)),
  topRight(createAnchor("topRight", aiTopRight)),
  right(createAnchor("right", aiRight)),
  bottomRight(createAnchor("bottomRight", aiBottomRight)),
  bottom(createAnchor("bottom", aiBottom)),
  bottomLeft(createAnchor("bottomLeft", aiBottomLeft)),
  left(createAnchor("left", aiLeft))
{
  position->setCoords(0, 0);

  setRotation(0);
  setTextAlignment(Qt::AlignTop|Qt::AlignHCenter);
  setPositionAlignment(Qt::AlignCenter);
  setText("text");

  setPen(Qt::NoPen);
  setSelectedPen(Qt::NoPen);
  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
  setColor(Qt::black);
  setSelectedColor(Qt::blue);
}

QCPItemText::~QCPItemText()
{
}

void QCPItemText::setColor(const QColor &color)
{
  mColor = color;
}

void QCPItemText::setSelectedColor(const QColor &color)
{
  mSelectedColor = color;
}

void QCPItemText::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemText::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemText::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPItemText::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

void QCPItemText::setFont(const QFont &font)
{
  mFont = font;
}

void QCPItemText::setSelectedFont(const QFont &font)
{
  mSelectedFont = font;
}

void QCPItemText::setText(const QString &text)
{
  mText = text;
}

void QCPItemText::setPositionAlignment(Qt::Alignment alignment)
{
  mPositionAlignment = alignment;
}

void QCPItemText::setTextAlignment(Qt::Alignment alignment)
{
  mTextAlignment = alignment;
}

void QCPItemText::setRotation(double degrees)
{
  mRotation = degrees;
}

void QCPItemText::setPadding(const QMargins &padding)
{
  mPadding = padding;
}

double QCPItemText::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QPointF positionPixels(position->pixelPoint());
  QTransform inputTransform;
  inputTransform.translate(positionPixels.x(), positionPixels.y());
  inputTransform.rotate(-mRotation);
  inputTransform.translate(-positionPixels.x(), -positionPixels.y());
  QPointF rotatedPos = inputTransform.map(pos);
  QFontMetrics fontMetrics(mFont);
  QRect textRect = fontMetrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip|mTextAlignment, mText);
  QRect textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());
  QPointF textPos = getTextDrawPoint(positionPixels, textBoxRect, mPositionAlignment);
  textBoxRect.moveTopLeft(textPos.toPoint());

  return rectSelectTest(textBoxRect, rotatedPos, true);
}

void QCPItemText::draw(QCPPainter *painter)
{
  QPointF pos(position->pixelPoint());
  QTransform transform = painter->transform();
  transform.translate(pos.x(), pos.y());
  if (!qFuzzyIsNull(mRotation))
    transform.rotate(mRotation);
  painter->setFont(mainFont());
  QRect textRect = painter->fontMetrics().boundingRect(0, 0, 0, 0, Qt::TextDontClip|mTextAlignment, mText);
  QRect textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());
  QPointF textPos = getTextDrawPoint(QPointF(0, 0), textBoxRect, mPositionAlignment);
  textRect.moveTopLeft(textPos.toPoint()+QPoint(mPadding.left(), mPadding.top()));
  textBoxRect.moveTopLeft(textPos.toPoint());
  double clipPad = mainPen().widthF();
  QRect boundingRect = textBoxRect.adjusted(-clipPad, -clipPad, clipPad, clipPad);
  if (transform.mapRect(boundingRect).intersects(painter->transform().mapRect(clipRect())))
  {
    painter->setTransform(transform);
    if ((mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0) ||
        (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0))
    {
      painter->setPen(mainPen());
      painter->setBrush(mainBrush());
      painter->drawRect(textBoxRect);
    }
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(mainColor()));
    painter->drawText(textRect, Qt::TextDontClip|mTextAlignment, mText);
  }
}

QPointF QCPItemText::anchorPixelPoint(int anchorId) const
{

  QPointF pos(position->pixelPoint());
  QTransform transform;
  transform.translate(pos.x(), pos.y());
  if (!qFuzzyIsNull(mRotation))
    transform.rotate(mRotation);
  QFontMetrics fontMetrics(mainFont());
  QRect textRect = fontMetrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip|mTextAlignment, mText);
  QRectF textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());
  QPointF textPos = getTextDrawPoint(QPointF(0, 0), textBoxRect, mPositionAlignment);
  textBoxRect.moveTopLeft(textPos.toPoint());
  QPolygonF rectPoly = transform.map(QPolygonF(textBoxRect));

  switch (anchorId)
  {
    case aiTopLeft:     return rectPoly.at(0);
    case aiTop:         return (rectPoly.at(0)+rectPoly.at(1))*0.5;
    case aiTopRight:    return rectPoly.at(1);
    case aiRight:       return (rectPoly.at(1)+rectPoly.at(2))*0.5;
    case aiBottomRight: return rectPoly.at(2);
    case aiBottom:      return (rectPoly.at(2)+rectPoly.at(3))*0.5;
    case aiBottomLeft:  return rectPoly.at(3);
    case aiLeft:        return (rectPoly.at(3)+rectPoly.at(0))*0.5;
  }

  qDebug() << Q_FUNC_INFO << "invalid anchorId" << anchorId;
  return QPointF();
}

QPointF QCPItemText::getTextDrawPoint(const QPointF &pos, const QRectF &rect, Qt::Alignment positionAlignment) const
{
  if (positionAlignment == 0 || positionAlignment == (Qt::AlignLeft|Qt::AlignTop))
    return pos;

  QPointF result = pos;
  if (positionAlignment.testFlag(Qt::AlignHCenter))
    result.rx() -= rect.width()/2.0;
  else if (positionAlignment.testFlag(Qt::AlignRight))
    result.rx() -= rect.width();
  if (positionAlignment.testFlag(Qt::AlignVCenter))
    result.ry() -= rect.height()/2.0;
  else if (positionAlignment.testFlag(Qt::AlignBottom))
    result.ry() -= rect.height();
  return result;
}

QFont QCPItemText::mainFont() const
{
  return mSelected ? mSelectedFont : mFont;
}

QColor QCPItemText::mainColor() const
{
  return mSelected ? mSelectedColor : mColor;
}

QPen QCPItemText::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPItemText::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}

QCPItemEllipse::QCPItemEllipse(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  topLeft(createPosition("topLeft")),
  bottomRight(createPosition("bottomRight")),
  topLeftRim(createAnchor("topLeftRim", aiTopLeftRim)),
  top(createAnchor("top", aiTop)),
  topRightRim(createAnchor("topRightRim", aiTopRightRim)),
  right(createAnchor("right", aiRight)),
  bottomRightRim(createAnchor("bottomRightRim", aiBottomRightRim)),
  bottom(createAnchor("bottom", aiBottom)),
  bottomLeftRim(createAnchor("bottomLeftRim", aiBottomLeftRim)),
  left(createAnchor("left", aiLeft)),
  center(createAnchor("center", aiCenter))
{
  topLeft->setCoords(0, 1);
  bottomRight->setCoords(1, 0);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue, 2));
  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
}

QCPItemEllipse::~QCPItemEllipse()
{
}

void QCPItemEllipse::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemEllipse::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemEllipse::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPItemEllipse::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

double QCPItemEllipse::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  double result = -1;
  QPointF p1 = topLeft->pixelPoint();
  QPointF p2 = bottomRight->pixelPoint();
  QPointF center((p1+p2)/2.0);
  double a = qAbs(p1.x()-p2.x())/2.0;
  double b = qAbs(p1.y()-p2.y())/2.0;
  double x = pos.x()-center.x();
  double y = pos.y()-center.y();

  double c = 1.0/qSqrt(x*x/(a*a)+y*y/(b*b));
  result = qAbs(c-1)*qSqrt(x*x+y*y);

  if (result > mParentPlot->selectionTolerance()*0.99 && mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0)
  {
    if (x*x/(a*a) + y*y/(b*b) <= 1)
      result = mParentPlot->selectionTolerance()*0.99;
  }
  return result;
}

void QCPItemEllipse::draw(QCPPainter *painter)
{
  QPointF p1 = topLeft->pixelPoint();
  QPointF p2 = bottomRight->pixelPoint();
  if (p1.toPoint() == p2.toPoint())
    return;
  QRectF ellipseRect = QRectF(p1, p2).normalized();
  QRect clip = clipRect().adjusted(-mainPen().widthF(), -mainPen().widthF(), mainPen().widthF(), mainPen().widthF());
  if (ellipseRect.intersects(clip))
  {
    painter->setPen(mainPen());
    painter->setBrush(mainBrush());
#ifdef __EXCEPTIONS
    try
    {
#endif
      painter->drawEllipse(ellipseRect);
#ifdef __EXCEPTIONS
    } catch (...)
    {
      qDebug() << Q_FUNC_INFO << "Item too large for memory, setting invisible";
      setVisible(false);
    }
#endif
  }
}

QPointF QCPItemEllipse::anchorPixelPoint(int anchorId) const
{
  QRectF rect = QRectF(topLeft->pixelPoint(), bottomRight->pixelPoint());
  switch (anchorId)
  {
    case aiTopLeftRim:     return rect.center()+(rect.topLeft()-rect.center())*1/qSqrt(2);
    case aiTop:            return (rect.topLeft()+rect.topRight())*0.5;
    case aiTopRightRim:    return rect.center()+(rect.topRight()-rect.center())*1/qSqrt(2);
    case aiRight:          return (rect.topRight()+rect.bottomRight())*0.5;
    case aiBottomRightRim: return rect.center()+(rect.bottomRight()-rect.center())*1/qSqrt(2);
    case aiBottom:         return (rect.bottomLeft()+rect.bottomRight())*0.5;
    case aiBottomLeftRim:  return rect.center()+(rect.bottomLeft()-rect.center())*1/qSqrt(2);
    case aiLeft:           return (rect.topLeft()+rect.bottomLeft())*0.5;
    case aiCenter:         return (rect.topLeft()+rect.bottomRight())*0.5;
  }

  qDebug() << Q_FUNC_INFO << "invalid anchorId" << anchorId;
  return QPointF();
}

QPen QCPItemEllipse::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPItemEllipse::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}

QCPItemPixmap::QCPItemPixmap(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  topLeft(createPosition("topLeft")),
  bottomRight(createPosition("bottomRight")),
  top(createAnchor("top", aiTop)),
  topRight(createAnchor("topRight", aiTopRight)),
  right(createAnchor("right", aiRight)),
  bottom(createAnchor("bottom", aiBottom)),
  bottomLeft(createAnchor("bottomLeft", aiBottomLeft)),
  left(createAnchor("left", aiLeft))
{
  topLeft->setCoords(0, 1);
  bottomRight->setCoords(1, 0);

  setPen(Qt::NoPen);
  setSelectedPen(QPen(Qt::blue));
  setScaled(false, Qt::KeepAspectRatio);
}

QCPItemPixmap::~QCPItemPixmap()
{
}

void QCPItemPixmap::setPixmap(const QPixmap &pixmap)
{
  mPixmap = pixmap;
  if (mPixmap.isNull())
    qDebug() << Q_FUNC_INFO << "pixmap is null";
}

void QCPItemPixmap::setScaled(bool scaled, Qt::AspectRatioMode aspectRatioMode)
{
  mScaled = scaled;
  mAspectRatioMode = aspectRatioMode;
  updateScaledPixmap();
}

void QCPItemPixmap::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemPixmap::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

double QCPItemPixmap::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  return rectSelectTest(getFinalRect(), pos, true);
}

void QCPItemPixmap::draw(QCPPainter *painter)
{
  bool flipHorz = false;
  bool flipVert = false;
  QRect rect = getFinalRect(&flipHorz, &flipVert);
  double clipPad = mainPen().style() == Qt::NoPen ? 0 : mainPen().widthF();
  QRect boundingRect = rect.adjusted(-clipPad, -clipPad, clipPad, clipPad);
  if (boundingRect.intersects(clipRect()))
  {
    updateScaledPixmap(rect, flipHorz, flipVert);
    painter->drawPixmap(rect.topLeft(), mScaled ? mScaledPixmap : mPixmap);
    QPen pen = mainPen();
    if (pen.style() != Qt::NoPen)
    {
      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(rect);
    }
  }
}

QPointF QCPItemPixmap::anchorPixelPoint(int anchorId) const
{
  bool flipHorz;
  bool flipVert;
  QRect rect = getFinalRect(&flipHorz, &flipVert);

  if (flipHorz)
    rect.adjust(rect.width(), 0, -rect.width(), 0);
  if (flipVert)
    rect.adjust(0, rect.height(), 0, -rect.height());

  switch (anchorId)
  {
    case aiTop:         return (rect.topLeft()+rect.topRight())*0.5;
    case aiTopRight:    return rect.topRight();
    case aiRight:       return (rect.topRight()+rect.bottomRight())*0.5;
    case aiBottom:      return (rect.bottomLeft()+rect.bottomRight())*0.5;
    case aiBottomLeft:  return rect.bottomLeft();
    case aiLeft:        return (rect.topLeft()+rect.bottomLeft())*0.5;;
  }

  qDebug() << Q_FUNC_INFO << "invalid anchorId" << anchorId;
  return QPointF();
}

void QCPItemPixmap::updateScaledPixmap(QRect finalRect, bool flipHorz, bool flipVert)
{
  if (mPixmap.isNull())
    return;

  if (mScaled)
  {
    if (finalRect.isNull())
      finalRect = getFinalRect(&flipHorz, &flipVert);
    if (finalRect.size() != mScaledPixmap.size())
    {
      mScaledPixmap = mPixmap.scaled(finalRect.size(), mAspectRatioMode, Qt::SmoothTransformation);
      if (flipHorz || flipVert)
        mScaledPixmap = QPixmap::fromImage(mScaledPixmap.toImage().mirrored(flipHorz, flipVert));
    }
  } else if (!mScaledPixmap.isNull())
    mScaledPixmap = QPixmap();
}

QRect QCPItemPixmap::getFinalRect(bool *flippedHorz, bool *flippedVert) const
{
  QRect result;
  bool flipHorz = false;
  bool flipVert = false;
  QPoint p1 = topLeft->pixelPoint().toPoint();
  QPoint p2 = bottomRight->pixelPoint().toPoint();
  if (p1 == p2)
    return QRect(p1, QSize(0, 0));
  if (mScaled)
  {
    QSize newSize = QSize(p2.x()-p1.x(), p2.y()-p1.y());
    QPoint topLeft = p1;
    if (newSize.width() < 0)
    {
      flipHorz = true;
      newSize.rwidth() *= -1;
      topLeft.setX(p2.x());
    }
    if (newSize.height() < 0)
    {
      flipVert = true;
      newSize.rheight() *= -1;
      topLeft.setY(p2.y());
    }
    QSize scaledSize = mPixmap.size();
    scaledSize.scale(newSize, mAspectRatioMode);
    result = QRect(topLeft, scaledSize);
  } else
  {
    result = QRect(p1, mPixmap.size());
  }
  if (flippedHorz)
    *flippedHorz = flipHorz;
  if (flippedVert)
    *flippedVert = flipVert;
  return result;
}

QPen QCPItemPixmap::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QCPItemTracer::QCPItemTracer(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  position(createPosition("position")),
  mGraph(0)
{
  position->setCoords(0, 0);

  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue, 2));
  setStyle(tsCrosshair);
  setSize(6);
  setInterpolating(false);
  setGraphKey(0);
}

QCPItemTracer::~QCPItemTracer()
{
}

void QCPItemTracer::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemTracer::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemTracer::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPItemTracer::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

void QCPItemTracer::setSize(double size)
{
  mSize = size;
}

void QCPItemTracer::setStyle(QCPItemTracer::TracerStyle style)
{
  mStyle = style;
}

void QCPItemTracer::setGraph(QCPGraph *graph)
{
  if (graph)
  {
    if (graph->parentPlot() == mParentPlot)
    {
      position->setType(QCPItemPosition::ptPlotCoords);
      position->setAxes(graph->keyAxis(), graph->valueAxis());
      mGraph = graph;
      updatePosition();
    } else
      qDebug() << Q_FUNC_INFO << "graph isn't in same QCustomPlot instance as this item";
  } else
  {
    mGraph = 0;
  }
}

void QCPItemTracer::setGraphKey(double key)
{
  mGraphKey = key;
}

void QCPItemTracer::setInterpolating(bool enabled)
{
  mInterpolating = enabled;
}

double QCPItemTracer::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QPointF center(position->pixelPoint());
  double w = mSize/2.0;
  QRect clip = clipRect();
  switch (mStyle)
  {
    case tsNone: return -1;
    case tsPlus:
    {
      if (clipRect().intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        return qSqrt(qMin(distSqrToLine(center+QPointF(-w, 0), center+QPointF(w, 0), pos),
                          distSqrToLine(center+QPointF(0, -w), center+QPointF(0, w), pos)));
      break;
    }
    case tsCrosshair:
    {
      return qSqrt(qMin(distSqrToLine(QPointF(clip.left(), center.y()), QPointF(clip.right(), center.y()), pos),
                        distSqrToLine(QPointF(center.x(), clip.top()), QPointF(center.x(), clip.bottom()), pos)));
    }
    case tsCircle:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {

        double centerDist = QVector2D(center-pos).length();
        double circleLine = w;
        double result = qAbs(centerDist-circleLine);

        if (result > mParentPlot->selectionTolerance()*0.99 && mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0)
        {
          if (centerDist <= circleLine)
            result = mParentPlot->selectionTolerance()*0.99;
        }
        return result;
      }
      break;
    }
    case tsSquare:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {
        QRectF rect = QRectF(center-QPointF(w, w), center+QPointF(w, w));
        bool filledRect = mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0;
        return rectSelectTest(rect, pos, filledRect);
      }
      break;
    }
  }
  return -1;
}

void QCPItemTracer::draw(QCPPainter *painter)
{
  updatePosition();
  if (mStyle == tsNone)
    return;

  painter->setPen(mainPen());
  painter->setBrush(mainBrush());
  QPointF center(position->pixelPoint());
  double w = mSize/2.0;
  QRect clip = clipRect();
  switch (mStyle)
  {
    case tsNone: return;
    case tsPlus:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {
        painter->drawLine(QLineF(center+QPointF(-w, 0), center+QPointF(w, 0)));
        painter->drawLine(QLineF(center+QPointF(0, -w), center+QPointF(0, w)));
      }
      break;
    }
    case tsCrosshair:
    {
      if (center.y() > clip.top() && center.y() < clip.bottom())
        painter->drawLine(QLineF(clip.left(), center.y(), clip.right(), center.y()));
      if (center.x() > clip.left() && center.x() < clip.right())
        painter->drawLine(QLineF(center.x(), clip.top(), center.x(), clip.bottom()));
      break;
    }
    case tsCircle:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        painter->drawEllipse(center, w, w);
      break;
    }
    case tsSquare:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        painter->drawRect(QRectF(center-QPointF(w, w), center+QPointF(w, w)));
      break;
    }
  }
}

void QCPItemTracer::updatePosition()
{
  if (mGraph)
  {
    if (mParentPlot->hasPlottable(mGraph))
    {
      if (mGraph->data()->size() > 1)
      {
        QCPDataMap::const_iterator first = mGraph->data()->constBegin();
        QCPDataMap::const_iterator last = mGraph->data()->constEnd()-1;
        if (mGraphKey < first.key())
          position->setCoords(first.key(), first.value().value);
        else if (mGraphKey > last.key())
          position->setCoords(last.key(), last.value().value);
        else
        {
          QCPDataMap::const_iterator it = mGraph->data()->lowerBound(mGraphKey);
          if (it != first)
          {
            QCPDataMap::const_iterator prevIt = it-1;
            if (mInterpolating)
            {

              double slope = 0;
              if (!qFuzzyCompare((double)it.key(), (double)prevIt.key()))
                slope = (it.value().value-prevIt.value().value)/(it.key()-prevIt.key());
              position->setCoords(mGraphKey, (mGraphKey-prevIt.key())*slope+prevIt.value().value);
            } else
            {

              if (mGraphKey < (prevIt.key()+it.key())*0.5)
                it = prevIt;
              position->setCoords(it.key(), it.value().value);
            }
          } else
            position->setCoords(it.key(), it.value().value);
        }
      } else if (mGraph->data()->size() == 1)
      {
        QCPDataMap::const_iterator it = mGraph->data()->constBegin();
        position->setCoords(it.key(), it.value().value);
      } else
        qDebug() << Q_FUNC_INFO << "graph has no data";
    } else
      qDebug() << Q_FUNC_INFO << "graph not contained in QCustomPlot instance (anymore)";
  }
}

QPen QCPItemTracer::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPItemTracer::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}

QCPItemBracket::QCPItemBracket(QCustomPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  left(createPosition("left")),
  right(createPosition("right")),
  center(createAnchor("center", aiCenter))
{
  left->setCoords(0, 0);
  right->setCoords(1, 1);

  setPen(QPen(Qt::black));
  setSelectedPen(QPen(Qt::blue, 2));
  setLength(8);
  setStyle(bsCalligraphic);
}

QCPItemBracket::~QCPItemBracket()
{
}

void QCPItemBracket::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPItemBracket::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPItemBracket::setLength(double length)
{
  mLength = length;
}

void QCPItemBracket::setStyle(QCPItemBracket::BracketStyle style)
{
  mStyle = style;
}

double QCPItemBracket::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QVector2D leftVec(left->pixelPoint());
  QVector2D rightVec(right->pixelPoint());
  if (leftVec.toPoint() == rightVec.toPoint())
    return -1;

  QVector2D widthVec = (rightVec-leftVec)*0.5f;
  QVector2D lengthVec(-widthVec.y(), widthVec.x());
  lengthVec = lengthVec.normalized()*mLength;
  QVector2D centerVec = (rightVec+leftVec)*0.5f-lengthVec;

  return qSqrt(distSqrToLine((centerVec-widthVec).toPointF(), (centerVec+widthVec).toPointF(), pos));
}

void QCPItemBracket::draw(QCPPainter *painter)
{
  QVector2D leftVec(left->pixelPoint());
  QVector2D rightVec(right->pixelPoint());
  if (leftVec.toPoint() == rightVec.toPoint())
    return;

  QVector2D widthVec = (rightVec-leftVec)*0.5f;
  QVector2D lengthVec(-widthVec.y(), widthVec.x());
  lengthVec = lengthVec.normalized()*mLength;
  QVector2D centerVec = (rightVec+leftVec)*0.5f-lengthVec;

  QPolygon boundingPoly;
  boundingPoly << leftVec.toPoint() << rightVec.toPoint()
               << (rightVec-lengthVec).toPoint() << (leftVec-lengthVec).toPoint();
  QRect clip = clipRect().adjusted(-mainPen().widthF(), -mainPen().widthF(), mainPen().widthF(), mainPen().widthF());
  if (clip.intersects(boundingPoly.boundingRect()))
  {
    painter->setPen(mainPen());
    switch (mStyle)
    {
      case bsSquare:
      {
        painter->drawLine((centerVec+widthVec).toPointF(), (centerVec-widthVec).toPointF());
        painter->drawLine((centerVec+widthVec).toPointF(), (centerVec+widthVec+lengthVec).toPointF());
        painter->drawLine((centerVec-widthVec).toPointF(), (centerVec-widthVec+lengthVec).toPointF());
        break;
      }
      case bsRound:
      {
        painter->setBrush(Qt::NoBrush);
        QPainterPath path;
        path.moveTo((centerVec+widthVec+lengthVec).toPointF());
        path.cubicTo((centerVec+widthVec).toPointF(), (centerVec+widthVec).toPointF(), centerVec.toPointF());
        path.cubicTo((centerVec-widthVec).toPointF(), (centerVec-widthVec).toPointF(), (centerVec-widthVec+lengthVec).toPointF());
        painter->drawPath(path);
        break;
      }
      case bsCurly:
      {
        painter->setBrush(Qt::NoBrush);
        QPainterPath path;
        path.moveTo((centerVec+widthVec+lengthVec).toPointF());
        path.cubicTo((centerVec+widthVec-lengthVec*0.8f).toPointF(), (centerVec+0.4f*widthVec+lengthVec).toPointF(), centerVec.toPointF());
        path.cubicTo((centerVec-0.4f*widthVec+lengthVec).toPointF(), (centerVec-widthVec-lengthVec*0.8f).toPointF(), (centerVec-widthVec+lengthVec).toPointF());
        painter->drawPath(path);
        break;
      }
      case bsCalligraphic:
      {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(mainPen().color()));
        QPainterPath path;
        path.moveTo((centerVec+widthVec+lengthVec).toPointF());

        path.cubicTo((centerVec+widthVec-lengthVec*0.8f).toPointF(), (centerVec+0.4f*widthVec+0.8f*lengthVec).toPointF(), centerVec.toPointF());
        path.cubicTo((centerVec-0.4f*widthVec+0.8f*lengthVec).toPointF(), (centerVec-widthVec-lengthVec*0.8f).toPointF(), (centerVec-widthVec+lengthVec).toPointF());

        path.cubicTo((centerVec-widthVec-lengthVec*0.5f).toPointF(), (centerVec-0.2f*widthVec+1.2f*lengthVec).toPointF(), (centerVec+lengthVec*0.2f).toPointF());
        path.cubicTo((centerVec+0.2f*widthVec+1.2f*lengthVec).toPointF(), (centerVec+widthVec-lengthVec*0.5f).toPointF(), (centerVec+widthVec+lengthVec).toPointF());

        painter->drawPath(path);
        break;
      }
    }
  }
}

QPointF QCPItemBracket::anchorPixelPoint(int anchorId) const
{
  QVector2D leftVec(left->pixelPoint());
  QVector2D rightVec(right->pixelPoint());
  if (leftVec.toPoint() == rightVec.toPoint())
    return leftVec.toPointF();

  QVector2D widthVec = (rightVec-leftVec)*0.5f;
  QVector2D lengthVec(-widthVec.y(), widthVec.x());
  lengthVec = lengthVec.normalized()*mLength;
  QVector2D centerVec = (rightVec+leftVec)*0.5f-lengthVec;

  switch (anchorId)
  {
    case aiCenter:
      return centerVec.toPointF();
  }
  qDebug() << Q_FUNC_INFO << "invalid anchorId" << anchorId;
  return QPointF();
}

QPen QCPItemBracket::mainPen() const
{
    return mSelected ? mSelectedPen : mPen;
}
