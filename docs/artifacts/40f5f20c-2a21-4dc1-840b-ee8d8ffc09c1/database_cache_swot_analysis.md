# Thumbnail Cache Index: Database SWOT Analysis

This analysis evaluates 10 different database and caching storage solutions for managing a 100k+ thumbnail cache in `appScrollBench`. The goal is to maximize read/write performance while minimizing external dependencies (plumbing) and overhead.

## 1. QDataStream + QHash (Native Qt Binary Serialization)
*The "Zero Plumbing" Native Approach. Loads a `QHash` into RAM on startup and dumps to a `.db` binary file on exit.*
* **Strengths:** 0 external dependencies. Fully native to Qt. Reads are literal memory lookups (`O(1)`). Very fast to write (a 100k item QHash writes to disk in milliseconds). 
* **Weaknesses:** Must hold the entire index in RAM (though at 100k items, this is only ~10-15MB). 
* **Opportunities:** Simplest possible plumbing. We can implement this in 20 lines of code.
* **Threats:** If the application crashes before writing to disk, the index could lose the most recent additions.

## 2. SQLite (via QSqlDatabase)
*The Industry Standard Embedded Relational DB.*
* **Strengths:** Ships natively with Qt. Fully ACID compliant. Crash-proof. Great for complex querying (e.g., `ORDER BY last_accessed LIMIT 100`).
* **Weaknesses:** "Heavy" for a simple Key-Value cache. Requires SQL string generation. Writes are bottlenecked by disk syncs unless wrapped in transactions.
* **Opportunities:** Very robust LRU pruning using standard SQL.
* **Threats:** SQL overhead might cause latency spikes on the worker threads if lock contention occurs during heavy writes.

## 3. LMDB (Lightning Memory-Mapped Database)
*Ultra-fast, zero-copy embedded Key-Value store.*
* **Strengths:** Mind-boggling read speeds (microseconds). Scales effortlessly to billions of records. Memory-mapped directly to the OS.
* **Weaknesses:** Requires adding external C-libraries and headers to the build pipeline.
* **Opportunities:** Perfect for read-heavy workloads like scrolling through a 100k photo grid.
* **Threats:** Plumbing and compiler setup can be annoying on Windows/MSVC/MinGW.

## 4. LevelDB
*Google's embedded Key-Value store designed for fast writes.*
* **Strengths:** Extremely fast writes due to Log-Structured Merge (LSM) trees. Excellent for appending thousands of thumbnails rapidly.
* **Weaknesses:** Reads can be slower than LMDB. Requires external C++ library compilation.
* **Opportunities:** Great if we anticipate background scanners ingesting tens of thousands of thumbnails simultaneously.
* **Threats:** Compilation on Windows often requires CMake tinkering and zlib/snappy dependencies.

## 5. JSON / Flat Text File
*Simple `index.json` text file.*
* **Strengths:** Human readable. Easy to debug. Zero dependencies.
* **Weaknesses:** Incredibly slow to parse at 100k records. Horrible write performance (must rewrite the entire file to update one entry).
* **Opportunities:** Good for prototyping or configuration, but not for high-frequency cache indexes.
* **Threats:** Unusable performance degradation as the library approaches 10,000+ items.

## 6. Bespoke Memory-Mapped Binary (Custom `.db`)
*Custom C++ struct array written directly to a `.bin` file on disk.*
* **Strengths:** Ultimate maximum performance. OS handles all caching. Zero dependencies.
* **Weaknesses:** We have to build the database engine ourselves (handling fragmentation, deletes, and resizing).
* **Opportunities:** We have total control over memory layouts and CPU cache-lines.
* **Threats:** Extremely high risk of bugs, corruption, and wasted development time reinventing the wheel.

## 7. RocksDB
*Facebook's multi-threaded evolution of LevelDB.*
* **Strengths:** Designed for SSDs and massive multi-threaded workloads. Incredible performance.
* **Weaknesses:** Absolute overkill. The library itself is massive (megabytes of compiled code).
* **Opportunities:** If the app scaled to cloud-storage levels.
* **Threats:** Way too "heavy" for a desktop application cache.

## 8. DuckDB
*Embedded analytical SQL database.*
* **Strengths:** Insanely fast for aggregate queries (e.g., SUM(bytes)).
* **Weaknesses:** Optimized for column-heavy analytics, not fast single-row key-value lookups.
* **Opportunities:** Great if we wanted to build advanced statistics dashboards.
* **Threats:** Does not fit the Key-Value paradigm of an image cache.

## 9. Qt QSettings (INI / Registry)
*Using standard Qt settings to store key-value pairs.*
* **Strengths:** Zero plumbing.
* **Weaknesses:** Horrendously slow for massive datasets. The Windows Registry or INI parsers are not designed to hold 100,000 keys.
* **Opportunities:** N/A.
* **Threats:** Will permanently freeze the application during serialization.

## 10. File System Determinism (Current Approach)
*No database index. Relying purely on deterministic paths and `QFile::exists()`.*
* **Strengths:** Zero plumbing. No index to corrupt.
* **Weaknesses:** At 100k files, NTFS Master File Table (MFT) lookups get slow. Pruning requires a full recursive directory scan.
* **Opportunities:** Highly resilient.
* **Threats:** Pruning a 50GB folder to find the "oldest" files requires opening 100k file handles, which will choke the disk I/O.

---

### **Recommendation**
If your goal is **Zero Plumbing** and a **Lightweight footprint**, we should use **Option 1: QDataStream + QHash**. 

We simply maintain a C++ `QHash<QString, CacheEntry>` in memory. Qt natively serializes this directly to a binary `.db` file in 3 lines of code. It uses no external libraries, requires no SQL, and read speeds are instantaneous. To prevent data loss, we just run a background `QTimer` that silently flushes the `QHash` to disk every 60 seconds if changes occurred.
