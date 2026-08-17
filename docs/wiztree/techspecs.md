Technical Specification: High-Performance Dual-Engine Storage Analysis Architecture

1. Executive System Architecture and the Dual-Engine Mandate

In the landscape of modern enterprise storage, sub-second file system analysis has transitioned from a specialized forensic capability to a core requirement for effective capacity management. Traditional recursive directory traversal methods are fundamentally unsuitable for volumes containing millions of objects due to the cumulative latency of high-level API calls and kernel-mode transitions.

The architecture must implement a bifurcated execution path to resolve the disparate bottlenecks of local throughput and network latency:

1. Strategic Disparity: The shift from a latency-bound model to a throughput-bound model is essential.
  * Legacy APIs (e.g., FindFirstFile): Bound by the latency of sequential, individual file requests. Legacy traversals (such as those used by WinDirStat) can require up to 18 minutes to scan a single large volume.
  * Direct Disk Access: Throughput-bound. By bypassing the ntfs.sys driver, the system reaches the hardware’s maximum read capability, reducing analysis time for 700k+ files to approximately 1 second.
2. The Runtime Dispatcher: The system must evaluate environmental context to select the engine. The dispatcher is required to verify the SE_MANAGE_VOLUME_NAME privilege (administrative rights) and the target path type.
  * Local Engine: Selected for local NTFS volumes when administrative credentials are present.
  * Network Engine: Selected for UNC paths or when the process lacks elevated privileges, utilizing optimized batching to mitigate SMB round-trip overhead.

This architectural split ensures that the analysis engine can leverage raw physical bandwidth where possible while maintaining the highest possible efficiency over latency-sensitive remote links.

2. Local Scanning Engine: Direct MFT Access and Volume Acquisition

To achieve maximum indexing performance, the local engine must bypass the Windows security subsystem and the standard ntfs.sys file system driver. This allows the system to stream metadata in raw blocks, avoiding the 4MB/s kernel-side translation bottleneck associated with the USN Journal.

2.1 Raw Volume Handle Acquisition

The application must establish a direct communication channel with the logical volume using CreateFileW.

* Path Syntax: The device path must use the exact syntax \\.\C: (where C: is the drive letter).
* Critical Flags: The handle must be opened with GENERIC_READ and sharing flags FILE_SHARE_READ | FILE_SHARE_WRITE. The FILE_FLAG_NO_BUFFERING flag is mandatory to bypass the OS cache manager and prefetching layers, ensuring a direct hardware-to-application data path.

2.2 NTFS Volume Geometry and Calculation

The system must query the volume's physical geometry via DeviceIoControl using FSCTL_GET_NTFS_VOLUME_DATA. This call populates the NTFS_VOLUME_DATA_BUFFER.

* Verification: The architect must mandate the verification of BytesPerFileRecordSegment. While typically 1024 bytes, this value must be confirmed to ensure the sector-aligned batching logic remains valid.
* MFT Offset Calculation: The absolute byte offset of the Master File Table (O_{\text{MFT}}) on the physical disk is calculated as: O_{\text{MFT}} = \text{MftStartLcn} \times \text{BytesPerCluster}

2.3 Superiority of Raw Parsing

Direct MFT parsing is mathematically superior to USN Journal enumeration (FSCTL_ENUM_USN_DATA). While USN journals facilitate simpler implementation, the kernel must translate internal records into specific USN structures, capping performance and extending a 1-second MFT scan into a 20-second USN enumeration.

Raw disk access ensures the binary integrity of retrieved records remains the responsibility of the application’s fixup routines.

3. MFT Record Processing and Sector Integrity Fixups

Each MFT record is a 1024-byte entry within a contiguous table. When bypassing the kernel, the application must perform user-space integrity checks to compensate for the lack of automated file system driver fixups.

3.1 Record Validation

The parsing routine must verify the "FILE" signature in the first four bytes of the header. If the signature is "BAAD", the record indicates a localized corruption or a failed sector write and must be flagged for recovery or skipped to maintain parser stability.

3.2 Update Sequence Number (USN) Sector Fixup

To detect partial sector writes (torn writes), the following fixup routine must be executed for every 1024-byte record:

1. Locate Array: Identify the UpdateSeqOffset in the header to find the Update Sequence Array.
2. Verify Boundary: Iterate through each sector boundary within the record (e.g., at the 511th/512th byte, etc.).
3. Signature Check: Compare the two-byte signature at these boundaries against the signature stored in the header.
4. Byte Restoration: If valid, replace the boundary bytes with the original bytes preserved in the Update Sequence Array.

3.3 Batch Reading and VirtualAlloc Mandate

To minimize I/O command queue overhead, records must be read in batches. The architecture mandates 128KB chunks (representing 128 records).

* Alignment Strategy: Because FILE_FLAG_NO_BUFFERING is active, the buffer must be strictly aligned to the physical sector boundary of the drive. Standard heap allocators (malloc/new) are prohibited here as they do not guarantee page-boundary alignment. The engine must use VirtualAlloc to provide sector-aligned memory for DMA-like transfers, eliminating the need for intermediate kernel-to-user copying.

Validated records provide the necessary pointers for resolving the physical mapping of file contents on the disk.

4. Non-Resident Attribute Resolution: Data Run Unpacking

Accurate size reporting requires the resolution of "data runs" for non-resident attributes—payloads fragmented across non-contiguous clusters.

4.1 Data Run Header Logic

Data runs are stored as a compressed stream. Each entry starts with a header byte used for nibble-based unpacking:

* Low Nibble (L): The count of bytes defining the run length (in clusters).
* High Nibble (F): The count of bytes defining the relative cluster offset.

4.2 Sign Extension and Assembly Logic

Offsets are signed values relative to the preceding run. Unpacking requires strict bit-twiddling logic:

1. Byte Assembly: Read F bytes and assemble them into a 64-bit buffer in little-endian order.
2. MSB Verification: Inspect the Most Significant Bit (MSB) of the highest byte actually read (the F-th byte).
3. Sign Extension: If the MSB is 1 (indicating a negative offset), the system must manually fill all leading bits up to the 64-bit boundary with 1s. This ensures a negative relative delta is correctly calculated as an absolute Logical Cluster Number (LCN).

4.3 Sparse and Compressed Units

The engine must distinguish between Virtual Cluster Numbers (VCNs) and Logical Cluster Numbers (LCNs). NTFS groups VCNs into 16-cluster "compression units."

* Sparse Units: If a unit consists entirely of zeroes, it is not physically stored. It is represented by a run where the offset length F=0. This indicates VCNs exist but consume zero physical space.
* Compressed Units: If a 16-VCN unit is stored in N clusters (N < 16), it is followed by a sparse run to round out the 16-cluster unit, ensuring the next unit begins at a correct VCN boundary.

5. Network Engine: Optimized Traversal and Latency Mitigation

Network scanning over UNC paths necessitates a shift from bandwidth-bound disk reading to latency-bound metadata batching.

5.1 SMB Batching and Handle Mandates

To minimize round-trip times (RTT), the engine must open directory handles with FILE_FLAG_BACKUP_SEMANTICS.

* API Usage: Metadata retrieval must utilize GetFileInformationByHandleEx with the FileIdBothDirectoryInfo class. This batches names, sizes, and file IDs into a single SMB packet, preventing the sequential RTT penalty of legacy calls.

5.2 Buffer Layout and Bypass Strategies

The network engine should utilize 64KB sector-aligned buffers to match the maximum payload boundaries of standard SMB protocol frames, maximizing entry density per packet.

* Namespace Bypass: When using FindFirstFileExW, the system must specify the FindExInfoBasic level. This directs the remote server to omit MS-DOS 8.3 short filename generation, reducing server-side hashing overhead and reducing the network payload size.

5.3 Antivirus and Filter Driver Bypass

Requesting low-level attribute blocks via direct handle queries avoids triggering "on-access" antivirus scans. This bypasses the directory-level security hooks that often degrade scanning speeds by several orders of magnitude.

6. High-Performance Concurrency and I/O Completion Ports (IOCP)

Managing millions of records requires a concurrency model that saturates available cores without the overhead of thread thrashing.

6.1 IOCP for Enterprise Scale

While the standard Windows ThreadPool is sufficient for small tasks, large-scale enterprise analysis (multiple remote servers, millions of files) requires I/O Completion Ports (IOCP). By associating handles with an IOCP and utilizing GetQueuedCompletionStatus, worker threads only wake when the OS has filled a buffer. This non-blocking asynchronous loop maximizes the I/O pipeline while preventing context-switching penalties.

6.2 Lock-Free Aggregate Calculation

The "hot path" of the scanner must be lock-free. Standard mutexes are prohibited for updating global counters. Instead, use atomic operations:

* InterlockedIncrement: For updating file/folder counts.
* InterlockedExchangeAdd64: For aggregating total file sizes across parallel threads.

7. Data-Oriented Memory Architecture (DOD)

Traditional Object-Oriented Design (OOP) and pointer-heavy trees fail at scale due to memory fragmentation and high cache-miss rates.

7.1 Structure of Arrays (SoA)

Metadata must be stored in contiguous pre-allocated vectors to ensure that data fields used together reside in the same CPU cache line.

* ParentIndices: 32-bit integers (index-based) rather than 64-bit pointers.
* FileSizes: Flat 64-bit array.
* String Pool: Filenames stored back-to-back in a raw character buffer to eliminate individual string object overhead.

7.2 Path Reconstruction and Hash Tables

Path reconstruction is performed by walking the index array using the 48-bit Parent File Reference Number (FRN).

* Amortized O(1) Complexity: The engine must utilize an open-addressed hash table with quadratic probing for FRN-to-index mapping. This avoids the O(\log n) pitfalls of balanced trees like std::map and ensures that the assembly phase remains a negligible portion of the total execution time.

8. Summary of Engineering Implementation Rules

Compliance with these constraints is mandatory for achieving sub-second analysis performance on local volumes with 700,000+ files.

Rule ID	Rule Name	Requirement
1	Aligned I/O	Must use VirtualAlloc for sector-aligned buffers; satisfies FILE_FLAG_NO_BUFFERING and avoids heap-copying.
2	Kernel Bypass	Must parse the MFT directly in user-space for local volumes to avoid the 4MB/s USN translation bottleneck.
3	Data-Oriented Design	Must utilize Structure of Arrays (SoA) and raw string pools to maximize CPU cache hit rates.
4	Batched Network I/O	Must use GetFileInformationByHandleEx with FILE_FLAG_BACKUP_SEMANTICS for remote scans to mitigate SMB latency.
5	Atomic Aggregation	Must use Interlocked atomic operations for size/count updates to ensure lock-free execution across high-count thread pools.
6	Geometry Verification	Must verify BytesPerFileRecordSegment from NTFS_VOLUME_DATA_BUFFER to validate parser record boundaries.

Adherence to this specification will result in an analysis architecture capable of near-instantaneous indexing on local physical volumes and high-efficiency traversal of remote network assets.


Technical Research Paper: High-Performance Disk Analysis via Direct Master File Table ($MFT) Parsing
Abstract
Modern disk analysis utilities often encounter performance bottlenecks when using standard operating system APIs to traverse file systems. This paper examines the architectural design of WizTree, a utility that achieves near-instantaneous scanning by bypassing traditional Windows file system drivers in favour of direct, low-level parsing of the NTFS Master File Table ($MFT)
. We detail the methodologies for raw volume access, the decoding of variable-length data runs, and the implementation of data-oriented memory structures
.
1. Introduction: The API Latency Bottleneck
Standard directory traversal (e.g., using FindFirstFile or std::filesystem) relies on recursive calls that process files individually
. This method introduces significant latency because:
Kernel-User Abstraction: Each file metadata query must pass through multiple OS layers
.
Filter Drivers: Real-time antivirus and security subsystems intercept each call, creating substantial overhead
.
WizTree circumvents these layers by treating the logical volume as a raw binary structure and reading the $MFT directly from the disk
.
2. Establishing Raw Volume Access
To bypass the OS file system driver (ntfs.sys), the application must acquire a raw handle to the logical volume, requiring elevated administrative privileges (SE_MANAGE_VOLUME_NAME)
.
2.1 Hardware-Direct I/O
A handle is opened using the CreateFileW API with the FILE_FLAG_NO_BUFFERING flag
. This instructs the OS cache manager to bypass intermediate caching, forcing direct physical sector reads into memory
.
// Opening a raw handle to the C: drive for sector-level access
HANDLE drive = CreateFileW(
    L"\\\\.\\C:", 
    GENERIC_READ, 
    FILE_SHARE_READ | FILE_SHARE_WRITE, 
    NULL, 
    OPEN_EXISTING, 
    FILE_FLAG_NO_BUFFERING, // Critical: Bypasses Windows system cache
    NULL
);
3. Locating and Parsing the $MFT
The $MFT is a relational database containing at least one 1,024-byte record for every file and folder on the volume
. Its location is not fixed but is defined in the volume's Boot Sector (the first 512 bytes)
.
3.1 Volume Geometry and MFT Offset
The absolute byte offset (O 
MFT
​
 ) is calculated by querying the volume's physical geometry using DeviceIoControl with the control code FSCTL_GET_NTFS_VOLUME_DATA
.
Calculation: 
O 
MFT
​
 =MftStartLcn×BytesPerCluster
3.2 ASCII Volume Layout
+---------------------------------------+
| Boot Sector (512 bytes)               |
| -> sectorsPerCluster                  |
| -> bytesPerSector                     |
| -> mftStartLcn  ----------------------+-----> [ Points to $MFT ]
+---------------------------------------+      |
| Unallocated / Data Clusters           |      |
+---------------------------------------+      |
| $MFT (Master File Table)              |<-----+
| [ Record 0: $MFT (Self-Reference)   ] |
| [ Record 1: $MFTMirr                ] |
| [ Record N: User File Metadata      ] |
+---------------------------------------+
4. Technical Deep Dive: Non-Resident Attributes and Data Runs
When a file's metadata or payload exceeds the 1,024-byte limit of a single record, it becomes non-resident
. The record then stores "data runs"—a compressed byte stream describing the physical clusters where the fragments reside
.
4.1 Data Run Structure
Each run element begins with a header byte. The low nibble (L) indicates the size of the length field, and the high nibble (F) indicates the size of the offset field
.
Header Byte (Hex): [ F | L ]
4.2 Sign Extension and Offset Calculation
Run offsets are relative deltas from the previous run and are stored as signed two's complement integers
. When converting these variable-length bytes to a 64-bit integer, the application must manually perform sign extension
.
// Logic for sign-extending a variable-length offset field
if (offset & (1ULL << (runHeader.offsetFieldBytes * 8 - 1))) {
    for (int i = runHeader.offsetFieldBytes; i < 8; i++) {
        offset |= (0xFFULL << (i * 8)); // Fill leading bits with 1s to preserve negative value
    }
}
clusterNumber += offset; // Cumulative absolute Logical Cluster Number (LCN)
``` [20, 21]

---

## 5. Optimization: Aligned Batch Reading and USN Fixups
### 5.1 Aligned Batch Reading
To maximize hardware throughput, WizTree reads MFT records in large, contiguous chunks (typically 128 records or 128 KB at once) [22, 23]. This reduces command queue overhead [22]. Because `FILE_FLAG_NO_BUFFERING` is used, the buffer must be strictly aligned to sector boundaries, often allocated via `VirtualAlloc` [23].

### 5.2 Update Sequence Number (USN) Sector Fixups
NTFS uses a "fixup" mechanism to detect partial sector writes [24]. Before parsing a record, the engine must:
1.  Read a 2-byte signature from the record header [25].
2.  Verify the last two bytes of every 512-byte sector in the record match this signature [25].
3.  Replace those bytes with the original values stored in the **Update Sequence Array** [25, 26].

---

## 6. In-Memory Database: Data-Oriented Design (DOD)
Traditional Object-Oriented Design (OOP) uses trees of pointers, which cause severe memory fragmentation and CPU cache misses [27, 28]. WizTree utilizes **Data-Oriented Design (SoA - Structure of Arrays)** [5, 29].

### 6.1 SoA Memory Layout
Metadata is stored in monolithic, contiguous vectors rather than discrete objects:
*   **ParentIndices:** A 32-bit integer array mapping child items to parents [5].
*   **FileSizes:** A flat array of 64-bit integers for storage tracking [5].
*   **StringPool:** A single continuous buffer for all filenames, indexed by 32-bit offsets [5].

This layout allows the engine to sort and visualize millions of records with linear memory scans, maximizing CPU cache efficiency [29, 30].

---

## 7. Network and Fallback Engine
When scanning network shares (SMB/UNC), raw sector access is prohibited [31]. In these cases, WizTree utilizes an optimized multi-threaded crawler using `GetFileInformationByHandleEx` with the `FileIdBothDirectoryInfo` class [32, 33]. This allows the application to retrieve metadata for hundreds of files in a single network packet, significantly reducing round-trip latency [33, 34].

---

## 8. Conclusion
WizTree's extreme performance is a direct result of bypassing high-level OS abstractions [9]. By combining **direct \$MFT parsing** with **variable-length bit unpacking** and **data-oriented memory management**, the tool indexes volumes containing 700,000 files in under one second—a 50x to 100x improvement over standard traversal methods [22, 35].