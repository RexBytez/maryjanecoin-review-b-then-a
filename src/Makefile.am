# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

# DIST_SUBDIRS = secp256k1

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(SANITIZER_LDFLAGS) $(CORE_LDFLAGS)
AM_CXXFLAGS = $(CORE_CXXFLAGS) $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_OBJCXXFLAGS = $(AM_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) $(CORE_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
check_PROGRAMS =
TESTS =
BENCHMARKS =

# T FUTURE BITCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT)
BITCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT)
BITCOIN_INCLUDES += $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(SSL_CFLAGS)

LIBBITCOIN_SERVER=libmaryjanecoin_server.a
LIBBITCOINQT_SERVER=libmaryjanecoinqt_server.a
# T LIBBITCOIN_COMMON=libmaryjanecoin_common.a
# T LIBBITCOIN_CONSENSUS=libmaryjanecoin_consensus.a
# T LIBBITCOIN_CLI=libmaryjanecoin_cli.a
LIBBITCOIN_UTIL=libmaryjanecoin_util.a
# T LIBBITCOIN_CRYPTO_BASE=crypto/libmaryjanecoin_crypto_base.a
LIBBITCOINQT=qt/libmaryjanecoinqt.a
# T LIBSECP256K1=secp256k1/libsecp256k1.la

# T if ENABLE_ZMQ
# T LIBBITCOIN_ZMQ=libmaryjanecoin_zmq.a
# T endif
# T if BUILD_BITCOIN_LIBS
# T LIBBITCOINCONSENSUS=libmaryjanecoinconsensus.la
# T endif
# T if ENABLE_WALLET
# T LIBBITCOIN_WALLET=libmaryjanecoin_wallet.a
# T LIBBITCOIN_WALLET_TOOL=libmaryjanecoin_wallet_tool.a
# T endif

# T LIBBITCOIN_CRYPTO= $(LIBBITCOIN_CRYPTO_BASE)
# T if ENABLE_SSE41
# T ls
# T LIBBITCOIN_CRYPTO_SSE41 = crypto/libmaryjanecoin_crypto_sse41.a
# T LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SSE41)
# T endif
# T if ENABLE_AVX2
# T LIBBITCOIN_CRYPTO_AVX2 = crypto/libmaryjanecoin_crypto_avx2.a
# T LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_AVX2)
# T endif
# T if ENABLE_SHANI
# T LIBBITCOIN_CRYPTO_SHANI = crypto/libmaryjanecoin_crypto_shani.a
# T LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SHANI)
# T endif

# T $(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
# T 	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
# T EXTRA_LIBRARIES += \
# T   $(LIBBITCOIN_CRYPTO) \
# T   $(LIBBITCOIN_UTIL) \
# T   $(LIBBITCOIN_COMMON) \
# T   $(LIBBITCOIN_CONSENSUS)

EXTRA_LIBRARIES += \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_SERVER)

EXTRA_LIBRARIES += \
  $(LIBBITCOINQT_SERVER)

# T EXTRA_LIBRARIES += \
# T   $(LIBBITCOIN_CLI) \
# T   $(LIBBITCOIN_IPC) \
# T   $(LIBBITCOIN_WALLET) \
# T   $(LIBBITCOIN_WALLET_TOOL) \
# T   $(LIBBITCOIN_ZMQ)

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
check_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_BITCOIND
  bin_PROGRAMS += maryjanecoind
endif

# T if BUILD_BITCOIN_NODE
# T   bin_PROGRAMS += maryjanecoin-node
# T endif

# T if BUILD_BITCOIN_CLI
# T   bin_PROGRAMS += maryjanecoin-cli
# T endif

# T if BUILD_BITCOIN_TX
# T   bin_PROGRAMS += maryjanecoin-tx
# T endif

# T if ENABLE_WALLET
# T if BUILD_BITCOIN_WALLET
# T   bin_PROGRAMS += maryjanecoin-wallet
# T endif
# T endif

# T if BUILD_BITCOIN_UTIL
# T   bin_PROGRAMS += maryjanecoin-util
# T endif

.PHONY: FORCE check-symbols check-security
# bitcoin core #
BITCOIN_CORE_H = \
  addrman.h \
  alert.h \
  base58.h \
  bignum.h \
  bitcoinrpc.h \
  bloom.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  compat.h \
  crypter.h \
  db.h \
  hash.h \
  hashblock.h \
  init.h \
  irc.h \
  json/json_spirit_writer_template.h \
  json/json_spirit_writer.h \
  json/json_spirit_value.h \
  json/json_spirit_utils.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_reader.h \
  json/json_spirit_error_position.h \
  json/json_spirit.h \
  kernel.h \
  key.h \
  keystore.h \
  limitedmap.h \
  main.h \
  miner.h \
  mruset.h \
  net.h \
  netbase.h \
  pbkdf2.h \
  protocol.h \
  script.h \
  scrypt.h \
  serialize.h \
  sph_blake.h \
  sph_bmw.h \
  sph_cubehash.h \
  sph_echo.h \
  sph_fugue.h \
  sph_groestl.h \
  sph_hamsi.h \
  sph_jh.h \
  sph_keccak.h \
  sph_luffa.h \
  sph_shavite.h \
  sph_simd.h \
  sph_skein.h \
  sph_types.h \
  strlcpy.h \
  support/cleanse.h \
  support/cleanse.cpp \
  support/lockedpool.h \
  support/lockedpool.cpp \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/allocators/pool.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  txdb-leveldb.h \
  txdb.h \
  ui_interface.h \
  uint256.h \
  uint256_t.h \
  util.h \
  version.h \
  wallet.h \
  walletdb.h  \
  util\macros.h
# T  addrdb.h \
# T  addrman.h \
# T  attributes.h \
# T  banman.h \
# T  base58.h \
# T  bech32.h \
# T  blockencodings.h \
# T  blockfilter.h \
# T  bloom.h \
# T  chain.h \
# T  chainparams.h \
# T  chainparamsbase.h \
# T  chainparamsseeds.h \
# T  checkqueue.h \
# T  clientversion.h \
# T  coins.h \
# T  compat.h \
# T  compat/assumptions.h \
# T  compat/byteswap.h \
# T  compat/cpuid.h \
# T  compat/endian.h \
# T  compat/sanity.h \
# T  compressor.h \
# T  consensus/consensus.h \
# T  consensus/tx_check.h \
# T  consensus/tx_verify.h \
# T  core_io.h \
# T  core_memusage.h \
# T  cuckoocache.h \
# T  dbwrapper.h \
# T  deploymentinfo.h \
# T  deploymentstatus.h \
# T  external_signer.h \
# T  flatfile.h \
# T  fs.h \
# T  httprpc.h \
# T  httpserver.h \
# T  i2p.h \
# T  index/base.h \
# T  index/blockfilterindex.h \
# T  index/coinstatsindex.h \
# T  index/disktxpos.h \
# T  index/txindex.h \
# T  indirectmap.h \
# T  init.h \
# T  init/common.h \
# T  interfaces/chain.h \
# T  interfaces/echo.h \
# T  interfaces/handler.h \
# T  interfaces/init.h \
# T  interfaces/ipc.h \
# T  interfaces/node.h \
# T  interfaces/wallet.h \
# T  key.h \
# T  key_io.h \
# T  logging.h \
# T  logging/timer.h \
# T  mapport.h \
# T  memusage.h \
# T  merkleblock.h \
# T  miner.h \
# T  net.h \
# T  net_permissions.h \
# T  net_processing.h \
# T  net_types.h \
# T  netaddress.h \
# T  netbase.h \
# T  netmessagemaker.h \
# T  node/blockstorage.h \
# T  node/coin.h \
# T  node/coinstats.h \
# T  node/context.h \
# T  node/psbt.h \
# T  node/transaction.h \
# T  node/ui_interface.h \
# T  node/utxo_snapshot.h \
# T  noui.h \
# T  outputtype.h \
# T  policy/feerate.h \
# T  policy/fees.h \
# T  policy/packages.h \
# T  policy/policy.h \
# T  policy/rbf.h \
# T  policy/settings.h \
# T  pow.h \
# T  protocol.h \
# T  psbt.h \
# T  random.h \
# T  randomenv.h \
# T  reverse_iterator.h \
# T  rpc/blockchain.h \
# T  rpc/client.h \
# T  rpc/mining.h \
# T  rpc/net.h \
# T  rpc/protocol.h \
# T  rpc/rawtransaction_util.h \
# T  rpc/register.h \
# T  rpc/request.h \
# T  rpc/server.h \
# T  rpc/util.h \
# T  scheduler.h \
# T  script/descriptor.h \
# T  script/keyorigin.h \
# T  script/sigcache.h \
# T  script/sign.h \
# T  script/signingprovider.h \
# T  script/standard.h \
# T  shutdown.h \
# T  signet.h \
# T  streams.h \
# T  support/allocators/secure.h \
# T  support/allocators/zeroafterfree.h \
# T  support/cleanse.h \
# T  support/events.h \
# T  support/lockedpool.h \
# T  sync.h \
# T  threadinterrupt.h \
# T  threadsafety.h \
# T  timedata.h \
# T  torcontrol.h \
# T  txdb.h \
# T  txmempool.h \
# T  txorphanage.h \
# T  txrequest.h \
# T  undo.h \
# T  util/asmap.h \
# T  util/bip32.h \
# T  util/bytevectorhash.h \
# T  util/check.h \
# T  util/epochguard.h \
# T  util/error.h \
# T  util/fees.h \
# T  util/getuniquepath.h \
# T  util/golombrice.h \
# T  util/hash_type.h \
# T  util/hasher.h \
# T  util/macros.h \
# T  util/message.h \
# T  util/moneystr.h \
# T  util/rbf.h \
# T  util/readwritefile.h \
# T  util/serfloat.h \
# T  util/settings.h \
# T  util/sock.h \
# T  util/spanparsing.h \
# T  util/string.h \
# T  util/system.h \
# T  util/thread.h \
# T  util/threadnames.h \
# T  util/time.h \
# T  util/tokenpipe.h \
# T  util/trace.h \
# T  util/translation.h \
# T  util/ui_change_type.h \
# T  util/url.h \
# T  util/vector.h \
# T  validation.h \
# T  validationinterface.h \
# T  versionbits.h \
# T  wallet/bdb.h \
# T  wallet/coincontrol.h \
# T  wallet/coinselection.h \
# T  wallet/context.h \
# T  wallet/crypter.h \
# T  wallet/db.h \
# T  wallet/dump.h \
# T  wallet/external_signer_scriptpubkeyman.h \
# T  wallet/feebumper.h \
# T  wallet/fees.h \
# T  wallet/ismine.h \
# T  wallet/load.h \
# T  wallet/receive.h \
# T  wallet/rpcwallet.h \
# T  wallet/salvage.h \
# T  wallet/scriptpubkeyman.h \
# T  wallet/spend.h \
# T  wallet/sqlite.h \
# T  wallet/transaction.h \
# T  wallet/wallet.h \
# T  wallet/walletdb.h \
# T  wallet/wallettool.h \
# T  wallet/walletutil.h \
# T  walletinitinterface.h \
# T  warnings.h \
# T  zmq/zmqabstractnotifier.h \
# T  zmq/zmqnotificationinterface.h \
# T  zmq/zmqpublishnotifier.h \
# T  zmq/zmqrpc.h \
# T  zmq/zmqutil.h


obj/build.h: FORCE
	@echo "generating build.h..."
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libmaryjanecoin_util_a-clientversion.$(OBJEXT): obj/build.h

# T ipc/capnp/libbitcoin_ipc_a-ipc.$(OBJEXT): $(libbitcoin_ipc_mpgen_input:=.h)

# server: shared between bitcoind and bitcoin-qt <---- NOT CURRENTLY THE CASE
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libbitcoin_common or libbitcoin_util libraries, instead.
libmaryjanecoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libmaryjanecoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmaryjanecoin_server_a_CFLAGS = $(AM_CFLAGS) $(PIE_FLAGS)
libmaryjanecoin_server_a_SOURCES = \
  aes_helper.c \
  blake.c \
  bmw.c \
  cubehash.c \
  echo.c \
  fugue.c \
  groestl.c \
  hamsi.c \
  hamsi_helper.c \
  jh.c \
  keccak.c \
  luffa.c \
  shavite.c \
  simd.c \
  skein.c \
  addrman.cpp \
  alert.cpp \
  bitcoinrpc.cpp \
  bloom.cpp \
  checkpoints.cpp \
  crypter.cpp \
  db.cpp \
  hash.cpp \
  init.cpp \
  irc.cpp \
  kernel.cpp \
  key.cpp \
  keystore.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  netbase.cpp \
  noui.cpp \
  pbkdf2.cpp \
  protocol.cpp \
  rpcblockchain.cpp \
  rpcdump.cpp \
  rpcmining.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcwallet.cpp \
  script.cpp \
  scrypt.cpp \
  sync.cpp \
  txdb-leveldb.cpp \
  util.cpp \
  wallet.cpp \
  walletdb.cpp \
  $(BITCOIN_CORE_H)





# T  addrdb.cpp \
# T  addrman.cpp \
# T  alert.cpp \
# T  banman.cpp \
# T  blockencodings.cpp \
# T  blockfilter.cpp \
# T  chain.cpp \
# T  consensus/tx_verify.cpp \
# T  dbwrapper.cpp \
# T  deploymentstatus.cpp \
# T  flatfile.cpp \
# T  httprpc.cpp \
# T  httpserver.cpp \
# T  i2p.cpp \
# T  index/base.cpp \
# T  index/blockfilterindex.cpp \
# T  index/coinstatsindex.cpp \
# T  index/txindex.cpp \
# T  init.cpp \
# T  mapport.cpp \
# T  miner.cpp \
# T  net.cpp \
# T  net_processing.cpp \
# T  node/blockstorage.cpp \
# T  node/coin.cpp \
# T  node/coinstats.cpp \
# T  node/context.cpp \
# T  node/interfaces.cpp \
# T  node/psbt.cpp \
# T  node/transaction.cpp \
# T  node/ui_interface.cpp \
# T  noui.cpp \
# T  policy/fees.cpp \
# T  policy/packages.cpp \
# T  policy/rbf.cpp \
# T  policy/settings.cpp \
# T  pow.cpp \
# T  rest.cpp \
# T  rpc/blockchain.cpp \
# T  rpc/mining.cpp \
# T  rpc/misc.cpp \
# T  rpc/net.cpp \
# T  rpc/rawtransaction.cpp \
# T  rpc/server.cpp \
# T  script/sigcache.cpp \
# T  shutdown.cpp \
# T  signet.cpp \
# T  timedata.cpp \
# T  torcontrol.cpp \
# T  txdb.cpp \
# T  txmempool.cpp \
# T  txorphanage.cpp \
# T  txrequest.cpp \
# T  validation.cpp \
# T  validationinterface.cpp \
# T  versionbits.cpp \
#  $(BITCOIN_CORE_H)

# T if ENABLE_WALLET
# T libmaryjanecoin_server_a_SOURCES += wallet/init.cpp
# T endif
# T if !ENABLE_WALLET
# T libmaryjanecoin_server_a_SOURCES += dummywallet.cpp
# T endif

# T if ENABLE_ZMQ
# T libmaryjanecoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(ZMQ_CFLAGS)
# T libmaryjanecoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_zmq_a_SOURCES = \
# T   zmq/zmqabstractnotifier.cpp \
# T   zmq/zmqnotificationinterface.cpp \
# T   zmq/zmqpublishnotifier.cpp \
# T   zmq/zmqrpc.cpp \
# T   zmq/zmqutil.cpp
# T endif


# T # wallet: shared between bitcoind and bitcoin-qt, but only linked
# T # when wallet enabled
# T libmaryjanecoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(SQLITE_CFLAGS)
# T libmaryjanecoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_wallet_a_SOURCES = \
# T   wallet/coincontrol.cpp \
# T   wallet/context.cpp \
# T   wallet/crypter.cpp \
# T   wallet/db.cpp \
# T   wallet/dump.cpp \
# T   wallet/external_signer_scriptpubkeyman.cpp \
# T   wallet/feebumper.cpp \
# T   wallet/fees.cpp \
# T   wallet/interfaces.cpp \
# T   wallet/load.cpp \
# T   wallet/receive.cpp \
# T   wallet/rpcdump.cpp \
# T   wallet/rpcwallet.cpp \
# T   wallet/scriptpubkeyman.cpp \
# T  wallet/spend.cpp \
# T  wallet/transaction.cpp \
# T  wallet/wallet.cpp \
# T  wallet/walletdb.cpp \
# T  wallet/walletutil.cpp \
# T  wallet/coinselection.cpp \
# T  $(BITCOIN_CORE_H)

# T if USE_SQLITE
# T libmaryjanecoin_wallet_a_SOURCES += wallet/sqlite.cpp
# T endif
# T if USE_BDB
# T libmaryjanecoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
# T endif

# T libmaryjanecoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T libmaryjanecoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_wallet_tool_a_SOURCES = \
# T   wallet/wallettool.cpp \
# T   $(BITCOIN_CORE_H)

# T TODO move our crypto here
# crypto primitives library
# T  crypto_libmaryjanecoin_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
# T  crypto_libmaryjanecoin_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T  crypto_libmaryjanecoin_crypto_base_a_SOURCES = \
# T  crypto/aes.cpp \
# T  crypto/aes.h \
# T  crypto/chacha_poly_aead.h \
# T  crypto/chacha_poly_aead.cpp \
# T  crypto/chacha20.h \
# T  crypto/chacha20.cpp \
# T  crypto/common.h \
# T  crypto/hkdf_sha256_32.cpp \
# T  crypto/hkdf_sha256_32.h \
# T  crypto/hmac_sha256.cpp \
# T  crypto/hmac_sha256.h \
# T  crypto/hmac_sha512.cpp \
# T  crypto/hmac_sha512.h \
# T  crypto/poly1305.h \
# T  crypto/poly1305.cpp \
# T  crypto/muhash.h \
# T  crypto/muhash.cpp \
# T  crypto/ripemd160.cpp \
# T  crypto/ripemd160.h \
# T  crypto/sha1.cpp \
# T  crypto/sha1.h \
# T  crypto/sha256.cpp \
# T  crypto/sha256.h \
# T  crypto/sha3.cpp \
# T  crypto/sha3.h \
# T  crypto/sha512.cpp \
# T  crypto/sha512.h \
# T  crypto/siphash.cpp \
# T  crypto/siphash.h

# T if USE_ASM
# T crypto_libmaryjanecoin_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
# T endif

# T crypto_libmaryjanecoin_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T crypto_libmaryjanecoin_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
# T crypto_libmaryjanecoin_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
# T crypto_libmaryjanecoin_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
# T crypto_libmaryjanecoin_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

# T crypto_libmaryjanecoin_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T crypto_libmaryjanecoin_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
# T crypto_libmaryjanecoin_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
# T crypto_libmaryjanecoin_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
# T crypto_libmaryjanecoin_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

# T crypto_libmaryjanecoin_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T crypto_libmaryjanecoin_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
# T crypto_libmaryjanecoin_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
# T crypto_libmaryjanecoin_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
# T crypto_libmaryjanecoin_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
# T libmaryjanecoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T libmaryjanecoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_consensus_a_SOURCES = \
# T   amount.h \
# T   arith_uint256.cpp \
# T   arith_uint256.h \
# T   consensus/merkle.cpp \
# T   consensus/merkle.h \
# T   consensus/params.h \
# T   consensus/tx_check.cpp \
# T   consensus/validation.h \
# T   hash.cpp \
# T   hash.h \
# T   prevector.h \
# T   primitives/block.cpp \
# T   primitives/block.h \
# T   primitives/transaction.cpp \
# T   primitives/transaction.h \
# T   pubkey.cpp \
# T   pubkey.h \
# T   script/bitcoinconsensus.cpp \
# T   script/interpreter.cpp \
# T   script/interpreter.h \
# T   script/script.cpp \
# T   script/script.h \
# T   script/script_error.cpp \
# T   script/script_error.h \
# T   serialize.h \
# T   span.h \
# T   tinyformat.h \
# T   uint256.cpp \
# T   uint256.h \
# T   util/strencodings.cpp \
# T   util/strencodings.h \
# T   version.h

# common: shared between bitcoind, and bitcoin-qt and non-server tools
# T libmaryjanecoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T libmaryjanecoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_common_a_SOURCES = \
# T   base58.cpp \
# T   bech32.cpp \
# T   bloom.cpp \
# T   chainparams.cpp \
# T   coins.cpp \
# T   compressor.cpp \
# T   core_read.cpp \
# T   core_write.cpp \
# T   deploymentinfo.cpp \
# T   external_signer.cpp \
# T   init/common.cpp \
# T   key.cpp \
# T   key_io.cpp \
# T   merkleblock.cpp \
# T   netaddress.cpp \
# T   netbase.cpp \
# T   net_permissions.cpp \
# T   outputtype.cpp \
# T   policy/feerate.cpp \
# T   policy/policy.cpp \
# T   protocol.cpp \
# T   psbt.cpp \
# T   rpc/rawtransaction_util.cpp \
# T   rpc/external_signer.cpp \
# T   rpc/util.cpp \
# T   scheduler.cpp \
# T   script/descriptor.cpp \
# T   script/sign.cpp \
# T   script/signingprovider.cpp \
# T   script/standard.cpp \
# T   warnings.cpp \
# T   $(BITCOIN_CORE_H)

# util: shared between all executables <-- NOT RIGHT NOW
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libmaryjanecoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libmaryjanecoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libmaryjanecoin_util_a_SOURCES = \
  clientversion.cpp \
  $(BITCOIN_CORE_H)

# T   support/lockedpool.cpp \
# T   chainparamsbase.cpp \
# T   clientversion.cpp \
# T   compat/glibcxx_sanity.cpp \
# T   compat/strnlen.cpp \
# T   fs.cpp \
# T   interfaces/echo.cpp \
# T   interfaces/handler.cpp \
# T   interfaces/init.cpp \
# T   logging.cpp \
# T   random.cpp \
# T   randomenv.cpp \
# T   rpc/request.cpp \
# T   support/cleanse.cpp \
# T   sync.cpp \
# T   threadinterrupt.cpp \
# T   util/asmap.cpp \
# T   util/bip32.cpp \
# T   util/bytevectorhash.cpp \
# T   util/error.cpp \
# T   util/fees.cpp \
# T   util/getuniquepath.cpp \
# T   util/hasher.cpp \
# T   util/sock.cpp \
# T   util/system.cpp \
# T   util/message.cpp \
# T   util/moneystr.cpp \
# T   util/rbf.cpp \
# T   util/readwritefile.cpp \
# T   util/settings.cpp \
# T   util/thread.cpp \
# T   util/threadnames.cpp \
# T   util/serfloat.cpp \
# T   util/spanparsing.cpp \
# T   util/strencodings.cpp \
# T   util/string.cpp \
# T   util/time.cpp \
# T   util/tokenpipe.cpp \
# T   $(BITCOIN_CORE_H)

# T if USE_LIBEVENT
# T libmaryjanecoin_util_a_SOURCES += util/url.cpp
# T endif

# T if GLIBC_BACK_COMPAT
# T libmaryjanecoin_util_a_SOURCES += compat/glibc_compat.cpp
# T AM_LDFLAGS += $(COMPAT_LDFLAGS)
# T endif

# T # cli: shared between bitcoin-cli and bitcoin-qt
# T libmaryjanecoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T libmaryjanecoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T libmaryjanecoin_cli_a_SOURCES = \
# T   compat/stdin.h \
# T   compat/stdin.cpp \
# T   rpc/client.cpp \
# T   $(BITCOIN_CORE_H)

nodist_libmaryjanecoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# maryjanecoind & bitcoin-node binaries #
maryjanecoin_daemon_sources = bitcoind.cpp
maryjanecoin_bin_cppflags = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
maryjanecoin_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
maryjanecoin_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
maryjanecoin_daemon_sources += maryjanecoind-res.rc
endif

maryjanecoin_bin_ldadd = \
  $(LIBBITCOIN_UTIL) \
  $(BOOST_LIBS) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(BDB_LIBS) \
  $(SSL_LIBS) \
  $(CRYPTO_LIBS) \
  $(MINIUPNPC_LIBS) \
  $(NATPMP_LIBS) \
  $(EVENT_PTHREADS_LIBS) \
  $(EVENT_LIBS) \
  $(ZMQ_LIBS) \
  $(SQLITE_LIBS)

if TARGET_LINUX
maryjanecoin_bin_ldadd += -lrt
endif
  
#maryjanecoin_bin_ldadd = \
#  $(LIBBITCOIN_UTIL)

# T maryjanecoin_bin_ldadd += \
# T   $(LIBBITCOIN_WALLET) \
# T   $(LIBBITCOIN_COMMON) \
# T   $(LIBBITCOIN_UTIL)

# maryjanecoin_bin_ldadd += \
#  $(LIBUNIVALUE)

# T maryjanecoin_bin_ldadd += \
# T   $(LIBBITCOIN_ZMQ) \
# T   $(LIBBITCOIN_CONSENSUS) \
# T   $(LIBBITCOIN_CRYPTO)

#maryjanecoin_bin_ldadd += \
#  $(LIBLEVELDB) \
#  $(LIBLEVELDB_SSE42)

#maryjanecoin_bin_ldadd += \
#  $(LIBMEMENV)

# maryjanecoin_bin_ldadd += \
#  $(LIBSECP256K1)

#maryjanecoin_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

# T maryjanecoind_SOURCES = $(maryjanecoin_daemon_sources) init/bitcoind.cpp
maryjanecoind_SOURCES = $(maryjanecoin_daemon_sources)
maryjanecoind_CPPFLAGS = $(maryjanecoin_bin_cppflags)
maryjanecoind_CXXFLAGS = $(maryjanecoin_bin_cxxflags)
maryjanecoind_LDFLAGS = $(maryjanecoin_bin_ldflags)
maryjanecoind_LDADD = $(LIBBITCOIN_SERVER) $(maryjanecoin_bin_ldadd)

# T bitcoin_node_SOURCES = $(bitcoin_daemon_sources) init/bitcoin-node.cpp
# T bitcoin_node_CPPFLAGS = $(bitcoin_bin_cppflags)
# T bitcoin_node_CXXFLAGS = $(bitcoin_bin_cxxflags)
# T bitcoin_node_LDFLAGS = $(bitcoin_bin_ldflags)
# T bitcoin_node_LDADD = $(LIBBITCOIN_SERVER) $(bitcoin_bin_ldadd) $(LIBBITCOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# T # bitcoin-cli binary #
# T bitcoin_cli_SOURCES = bitcoin-cli.cpp
# T bitcoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
# T bitcoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T bitcoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

# T if TARGET_WINDOWS
# T bitcoin_cli_SOURCES += bitcoin-cli-res.rc
# T endif

# T bitcoin_cli_LDADD = \
# T   $(LIBBITCOIN_CLI) \
# T   $(LIBBITCOIN_UTIL) \
# T   $(LIBBITCOIN_CRYPTO)

# T bitcoin_cli_LDADD += \
# T   $(LIBUNIVALUE)

# T bitcoin_cli_LDADD += $(BOOST_LIBS) $(EVENT_LIBS)
#

# T # bitcoin-tx binary #
# T bitcoin_tx_SOURCES = bitcoin-tx.cpp
# T bitcoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T bitcoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T bitcoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

# T if TARGET_WINDOWS
# T bitcoin_tx_SOURCES += bitcoin-tx-res.rc
# T endif

# T bitcoin_tx_LDADD = \
# T   $(LIBBITCOIN_COMMON) \
# T   $(LIBBITCOIN_UTIL) \
# T   $(LIBBITCOIN_CONSENSUS) \
# T   $(LIBBITCOIN_CRYPTO)

# T bitcoin_tx_LDADD += \
# T   $(LIBUNIVALUE) \
# T   $(LIBSECP256K1)

# T bitcoin_tx_LDADD += $(BOOST_LIBS)
#

# T # bitcoin-wallet binary #
# T bitcoin_wallet_SOURCES = bitcoin-wallet.cpp
# T bitcoin_wallet_CPPFLAGS = $(bitcoin_bin_cppflags)
# T bitcoin_wallet_CXXFLAGS = $(bitcoin_bin_cxxflags)
# T bitcoin_wallet_LDFLAGS = $(bitcoin_bin_ldflags)
# T bitcoin_wallet_LDADD = $(LIBBITCOIN_WALLET_TOOL) $(bitcoin_bin_ldadd)

# T if TARGET_WINDOWS
# T bitcoin_wallet_SOURCES += bitcoin-wallet-res.rc
# T endif
#

# T # bitcoin-util binary #
# T bitcoin_util_SOURCES = bitcoin-util.cpp
# T bitcoin_util_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T bitcoin_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
# T bitcoin_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

# T if TARGET_WINDOWS
# T bitcoin_util_SOURCES += bitcoin-util-res.rc
# T endif

# T bitcoin_util_LDADD = \
# T   $(LIBBITCOIN_COMMON) \
# T   $(LIBBITCOIN_UTIL) \
# T   $(LIBBITCOIN_CONSENSUS) \
# T   $(LIBBITCOIN_CRYPTO)

# T bitcoin_util_LDADD += \
# T   $(LIBUNIVALUE) \
# T   $(LIBSECP256K1)

# T bitcoin_util_LDADD += $(BOOST_LIBS)
#

# T # bitcoinconsensus library #
# T if BUILD_BITCOIN_LIBS
# T include_HEADERS = script/bitcoinconsensus.h
# T libbitcoinconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libbitcoin_crypto_base_a_SOURCES) $(libbitcoin_consensus_a_SOURCES)

# T if GLIBC_BACK_COMPAT
# T   libbitcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
# T endif

# T libbitcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
# T libbitcoinconsensus_la_LIBADD = $(LIBSECP256K1)
# T libbitcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BITCOIN_INTERNAL
# T libbitcoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

# T endif
#

# T CTAES_DIST =  crypto/ctaes/bench.c
# T CTAES_DIST += crypto/ctaes/ctaes.c
# T CTAES_DIST += crypto/ctaes/ctaes.h
# T CTAES_DIST += crypto/ctaes/README.md
# T CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/bitcoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/bitcoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/bitcoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/bitcoin-config.h.in

clean-local:
# T	-$(MAKE) -C secp256k1 clean
# T	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if TARGET_DARWIN
	@echo "Checking macOS dynamic libraries..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_WINDOWS
	@echo "Checking Windows dynamic libraries..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_LINUX
	@echo "Checking glibc back compat..."
	$(AM_V_at) CPPFILT='$(CPPFILT)' $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

# T libbitcoin_ipc_mpgen_input = \
# T   ipc/capnp/echo.capnp \
# T   ipc/capnp/init.capnp
# T EXTRA_DIST += $(libbitcoin_ipc_mpgen_input)
# T %.capnp:

# T if BUILD_MULTIPROCESS
# T LIBBITCOIN_IPC=libbitcoin_ipc.a
# T libbitcoin_ipc_a_SOURCES = \
# T   ipc/capnp/context.h \
# T   ipc/capnp/init-types.h \
# T   ipc/capnp/protocol.cpp \
# T   ipc/capnp/protocol.h \
# T   ipc/context.h \
# T   ipc/exception.h \
# T   ipc/interfaces.cpp \
# T   ipc/process.cpp \
# T   ipc/process.h \
# T   ipc/protocol.h
# T libbitcoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
# T libbitcoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

# T include $(MPGEN_PREFIX)/include/mpgen.mk
# T libbitcoin_ipc_mpgen_output = \
# T   $(libbitcoin_ipc_mpgen_input:=.c++) \
# T   $(libbitcoin_ipc_mpgen_input:=.h) \
# T   $(libbitcoin_ipc_mpgen_input:=.proxy-client.c++) \
# T   $(libbitcoin_ipc_mpgen_input:=.proxy-server.c++) \
# T   $(libbitcoin_ipc_mpgen_input:=.proxy-types.c++) \
# T   $(libbitcoin_ipc_mpgen_input:=.proxy-types.h) \
# T   $(libbitcoin_ipc_mpgen_input:=.proxy.h)
# T nodist_libbitcoin_ipc_a_SOURCES = $(libbitcoin_ipc_mpgen_output)
# T CLEANFILES += $(libbitcoin_ipc_mpgen_output)
# T endif

include Makefile.crc32c.include
include Makefile.leveldb.include

# T include Makefile.test_util.include
# T include Makefile.test_fuzz.include

if ENABLE_TESTS
include Makefile.test.include
endif

# T if ENABLE_BENCH
# T include Makefile.bench.include
# T endif

if ENABLE_QT
include Makefile.qt.include
endif

# T if ENABLE_QT_TESTS
# T include Makefile.qttest.include
# T endif
