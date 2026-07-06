#!/usr/bin/env python3
"""
generate-escrow-wallets.py — Creates MaryJaneCoin escrow address hierarchy on Pi4

Generates:
  - 100 root addresses (tier 0, accounts: root_1 .. root_100)
  - 10-15 child addresses per root (tier 1, accounts: escrow_N_M)
  - Total: 1100-1600 addresses

Stores all private keys encrypted (Fernet/AES-128-CBC + HMAC-SHA256)
in SQLite: escrow_keys.db
Also dumps full wallet backup via dumpwallet RPC.

Encryption: PBKDF2-SHA256 @ 480k iterations
Database:   ~/maryjanecoin-bridge/escrow_keys.db

Usage (run on Pi4):
  python3 generate-escrow-wallets.py --password "your-master-password"

Verify only (after generation):
  python3 generate-escrow-wallets.py --password "your-master-password" --verify-only
"""

import json
import subprocess
import random
import sqlite3
import hashlib
import base64
import os
import sys
import argparse
from datetime import datetime

try:
    from cryptography.fernet import Fernet
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
except ImportError:
    print("ERROR: Run: pip3 install cryptography")
    sys.exit(1)

# MaryJaneCoin-specific paths
RPC_CMD = os.path.expanduser("~/MaryJaneCoin-Build/src/MaryJaneCoind")
DB_PATH = os.path.expanduser("~/maryjanecoin-bridge/escrow_keys.db")
DUMP_PATH = os.path.expanduser("~/maryjanecoin-bridge/wallet-dump-escrow.txt")

# RPC connection settings (passed as CLI args to MaryJaneCoind)
RPC_ARGS = [
    "-rpcuser=maryjrpc",
    "-rpcpassword=maryjpass420pi4",
    "-rpcport=14201",
]

NUM_ROOTS = 100
CHILDREN_MIN = 10
CHILDREN_MAX = 15


def rpc(method, *params):
    """Call MaryJaneCoind RPC and return parsed result."""
    cmd = [RPC_CMD] + RPC_ARGS + [method] + [str(p) for p in params]
    try:
        result = subprocess.check_output(cmd, text=True, timeout=30).strip()
    except subprocess.CalledProcessError as e:
        print(f"RPC ERROR: {method} {params} -> {e}")
        raise
    try:
        return json.loads(result)
    except (json.JSONDecodeError, ValueError):
        return result


def derive_fernet_key(password: str, salt: bytes) -> Fernet:
    """Derive a Fernet encryption key from password + salt using PBKDF2."""
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=480000,
    )
    key = base64.urlsafe_b64encode(kdf.derive(password.encode()))
    return Fernet(key)


def create_db(db_path: str) -> sqlite3.Connection:
    """Create the escrow_keys database with schema."""
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA foreign_keys=ON")

    conn.executescript("""
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY,
            value TEXT
        );

        CREATE TABLE IF NOT EXISTS addresses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT NOT NULL,
            address TEXT NOT NULL UNIQUE,
            encrypted_privkey BLOB NOT NULL,
            parent_account TEXT,
            tier INTEGER NOT NULL,
            status TEXT DEFAULT 'empty',
            locked_by_tx TEXT,
            locked_at TIMESTAMP,
            balance REAL DEFAULT 0,
            last_updated TIMESTAMP
        );

        CREATE INDEX IF NOT EXISTS idx_status ON addresses(status);
        CREATE INDEX IF NOT EXISTS idx_tier ON addresses(tier);
        CREATE INDEX IF NOT EXISTS idx_parent ON addresses(parent_account);
        CREATE INDEX IF NOT EXISTS idx_address ON addresses(address);

        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            txid TEXT NOT NULL,
            from_account TEXT,
            to_address TEXT,
            amount REAL,
            direction TEXT,
            solana_tx TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    """)
    conn.commit()
    return conn


def generate_tree(password: str):
    """Generate full address hierarchy and store in encrypted DB."""

    # Check MaryJaneCoind is running
    try:
        info = rpc("getinfo")
        print(f"MaryJaneCoind running — block {info['blocks']}, keypool {info['keypoolsize']}")
    except Exception as e:
        print(f"FATAL: MaryJaneCoind not responding: {e}")
        print(f"Start the daemon first:")
        print(f"  {RPC_CMD}")
        sys.exit(1)

    if info["keypoolsize"] < NUM_ROOTS * CHILDREN_MAX + NUM_ROOTS:
        print(f"WARNING: Keypool ({info['keypoolsize']}) may be too small. Refilling...")
        rpc("keypoolrefill", 2000)

    # Setup encryption
    salt = os.urandom(16)
    fernet = derive_fernet_key(password, salt)

    # Verify encryption round-trip before generating any keys
    test_data = b"maryjanecoin-escrow-round-trip-test"
    assert fernet.decrypt(fernet.encrypt(test_data)) == test_data, "Encryption self-test FAILED!"

    # Create database (back up existing if present)
    if os.path.exists(DB_PATH):
        backup = DB_PATH + f".backup-{datetime.now().strftime('%Y%m%d-%H%M%S')}"
        os.rename(DB_PATH, backup)
        print(f"Existing DB backed up to {backup}")

    conn = create_db(DB_PATH)

    # Store metadata
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("encryption_salt", salt.hex()))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("created_at", datetime.utcnow().isoformat()))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("version", "1.0"))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("coin", "MARYJ"))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("chain", "MaryJaneCoin"))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("rpc_port", "14201"))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("num_roots", str(NUM_ROOTS)))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("children_min", str(CHILDREN_MIN)))
    conn.execute("INSERT INTO metadata VALUES (?, ?)", ("children_max", str(CHILDREN_MAX)))
    conn.commit()

    total_addresses = 0
    total_children = 0

    print(f"\n{'='*60}")
    print(f"Generating {NUM_ROOTS} root addresses with {CHILDREN_MIN}-{CHILDREN_MAX} children each")
    print(f"All MARYJ addresses start with 'M' (prefix byte 50)")
    print(f"{'='*60}\n")

    for i in range(1, NUM_ROOTS + 1):
        root_account = f"root_{i}"

        # Generate root address
        root_addr = rpc("getnewaddress", root_account)
        if not root_addr.startswith("M"):
            print(f"WARNING: root_{i} address {root_addr} does not start with 'M' — check address prefix config!")

        root_privkey = rpc("dumpprivkey", root_addr)

        # Encrypt and store root
        encrypted_key = fernet.encrypt(root_privkey.encode())
        conn.execute(
            "INSERT INTO addresses (account, address, encrypted_privkey, parent_account, tier, status) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (root_account, root_addr, encrypted_key, None, 0, "empty")
        )
        total_addresses += 1

        # Generate children
        num_children = random.randint(CHILDREN_MIN, CHILDREN_MAX)
        children_created = 0

        for j in range(1, num_children + 1):
            child_account = f"escrow_{i}_{j}"
            child_addr = rpc("getnewaddress", child_account)
            child_privkey = rpc("dumpprivkey", child_addr)

            encrypted_key = fernet.encrypt(child_privkey.encode())
            conn.execute(
                "INSERT INTO addresses (account, address, encrypted_privkey, parent_account, tier, status) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                (child_account, child_addr, encrypted_key, root_account, 1, "empty")
            )
            children_created += 1
            total_addresses += 1

        total_children += children_created

        # Commit every root (checkpoint — never lose more than one root's work)
        conn.commit()

        if i % 10 == 0 or i == NUM_ROOTS:
            print(f"  [{i:3d}/{NUM_ROOTS}] root_{i}: {root_addr} -> {children_created} children (total: {total_addresses})")

    # Store final counts
    conn.execute("INSERT OR REPLACE INTO metadata VALUES (?, ?)", ("total_addresses", str(total_addresses)))
    conn.execute("INSERT OR REPLACE INTO metadata VALUES (?, ?)", ("total_roots", str(NUM_ROOTS)))
    conn.execute("INSERT OR REPLACE INTO metadata VALUES (?, ?)", ("total_children", str(total_children)))
    conn.commit()

    # Verification: decrypt a random key to confirm round-trip
    row = conn.execute("SELECT address, encrypted_privkey FROM addresses ORDER BY RANDOM() LIMIT 1").fetchone()
    decrypted = fernet.decrypt(row["encrypted_privkey"]).decode()
    verify_addr = rpc("validateaddress", row["address"])
    is_valid = verify_addr.get("isvalid", False)
    print(f"\nVerification: decrypted key for {row['address']} — address valid: {is_valid}")
    if not is_valid:
        print("WARNING: Address validation failed — check chain is running correctly!")

    # Dump full wallet backup
    print(f"\nDumping wallet to {DUMP_PATH}...")
    try:
        rpc("dumpwallet", DUMP_PATH)
        print(f"Wallet dump saved: {DUMP_PATH}")
    except Exception as e:
        print(f"WARNING: dumpwallet failed (wallet may need unlocking): {e}")

    # Summary
    print(f"\n{'='*60}")
    print(f"GENERATION COMPLETE — MaryJaneCoin Escrow")
    print(f"{'='*60}")
    print(f"  Root addresses:   {NUM_ROOTS}")
    print(f"  Child addresses:  {total_children}")
    print(f"  Total addresses:  {total_addresses}")
    print(f"  Database:         {DB_PATH}")
    print(f"  DB size:          {os.path.getsize(DB_PATH) / 1024:.1f} KB")
    print(f"  Encryption:       Fernet (AES-128-CBC + HMAC-SHA256)")
    print(f"  KDF:              PBKDF2-SHA256, 480k iterations")
    print(f"  Salt:             {salt.hex()}")
    print(f"")
    print(f"  IMPORTANT: Back up {DB_PATH} and ~/.MaryJaneCoin/wallet.dat to WSL immediately!")
    print(f"  IMPORTANT: {DUMP_PATH} contains PLAINTEXT private keys — secure or delete after backup!")

    conn.close()
    return total_addresses


def verify_db(password: str):
    """Verify the encrypted database can be read back correctly."""
    if not os.path.exists(DB_PATH):
        print(f"ERROR: {DB_PATH} not found. Run without --verify-only first.")
        return

    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row

    salt_hex = conn.execute("SELECT value FROM metadata WHERE key='encryption_salt'").fetchone()["value"]
    salt = bytes.fromhex(salt_hex)
    fernet = derive_fernet_key(password, salt)

    total = conn.execute("SELECT COUNT(*) as c FROM addresses").fetchone()["c"]
    roots = conn.execute("SELECT COUNT(*) as c FROM addresses WHERE tier=0").fetchone()["c"]
    children = conn.execute("SELECT COUNT(*) as c FROM addresses WHERE tier=1").fetchone()["c"]

    # Metadata
    meta_rows = conn.execute("SELECT key, value FROM metadata ORDER BY key").fetchall()
    print("Database metadata:")
    for row in meta_rows:
        print(f"  {row['key']:20s}: {row['value']}")

    print(f"\nAddress counts: {total} total ({roots} roots, {children} children)")

    # Decrypt 5 random keys to verify
    rows = conn.execute(
        "SELECT account, address, encrypted_privkey FROM addresses ORDER BY RANDOM() LIMIT 5"
    ).fetchall()
    print("\nRandom key decryption check (5 samples):")
    all_ok = True
    for row in rows:
        try:
            decrypted = fernet.decrypt(row["encrypted_privkey"]).decode()
            print(f"  OK:   {row['account']:20s}  {row['address']}  (key: {decrypted[:8]}...)")
        except Exception as e:
            print(f"  FAIL: {row['account']:20s}  {row['address']}  -> {e}")
            all_ok = False

    if all_ok:
        print("\nAll checks passed.")
    else:
        print("\nERROR: Some decryptions failed — wrong password or corrupted DB?")

    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate MaryJaneCoin escrow wallet hierarchy on Pi4"
    )
    parser.add_argument(
        "--password",
        required=True,
        help="Master encryption password for private keys"
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="Only verify existing DB integrity, don't generate new addresses"
    )
    args = parser.parse_args()

    if args.verify_only:
        verify_db(args.password)
    else:
        generate_tree(args.password)
