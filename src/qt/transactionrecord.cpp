#include "transactionrecord.h"

#include "wallet.h"
#include "base58.h"

bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {

        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }
    return true;
}

QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    int64_t nCredit = wtx.GetCredit(true);
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (wtx.IsCoinStake())
    {

        int64_t nActualCredit = 0;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if(wallet->IsMine(txout))
            {
                nActualCredit += wallet->GetCredit(txout);
            }
        }
        TransactionRecord txrCoinStake = TransactionRecord(hash, nTime, TransactionRecord::StakeMint, "", -nDebit, nActualCredit);

		CTxDestination address;
		int64_t nLargestValue = 0;
		BOOST_FOREACH(const CTxOut& txout, wtx.vout)
		{
			if(wallet->IsMine(txout))
			{

				if (txout.nValue > nLargestValue)
				{
					if (ExtractDestination(txout.scriptPubKey, address))
					{
						txrCoinStake.address = CBitcoinAddress(address).ToString();
						nLargestValue = txout.nValue;
					}
				}
			}
		}

        parts.append(txrCoinStake);
    }
    else if (nNet > 0 || wtx.IsCoinBase())
    {

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if(wallet->IsMine(txout))
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = parts.size();
                sub.credit = txout.nValue;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {

                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                }
                else
                {

                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase())
                {

                    sub.type = TransactionRecord::Generated;
                }

                parts.append(sub);
            }
        }
    }
    else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && wallet->IsMine(txin);

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && wallet->IsMine(txout);

        if (fAllFromMe && fAllToMe)
        {

            int64_t nChange = wtx.GetChange();
			TransactionRecord sub(hash, nTime);
			sub.type = TransactionRecord::SendToSelf;
			sub.credit = nCredit - nChange;
			sub.debit =  -(nDebit - nChange);
			CTxDestination address;
			if (ExtractDestination(wtx.vout[0].scriptPubKey, address))
				 sub.address = CBitcoinAddress(address).ToString();
			parts.append(sub);
        }
        else if (fAllFromMe)
        {

            int64_t nTxFee = nDebit - wtx.GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();

                if(wallet->IsMine(txout))
                {

                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {

                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                }
                else
                {

                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                int64_t nValue = txout.nValue;

                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.append(sub);
            }
        }
        else
        {

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{

    CBlockIndex* pindex = NULL;
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.confirmed = wtx.IsConfirmed();
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = nBestHeight;

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    else
    {
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Unconfirmed;
        }
        else
        {
            status.status = TransactionStatus::HaveConfirmations;
        }
    }

 if(type == TransactionRecord::Generated || type == TransactionRecord::StakeMint)
    {
        int64_t nCredit = wtx.GetCredit(true);
        if (nCredit == 0)
        {
            status.maturity = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.maturity = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.maturity = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.maturity = TransactionStatus::Mature;
        }
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    return status.cur_num_blocks != nBestHeight;
}

std::string TransactionRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}
