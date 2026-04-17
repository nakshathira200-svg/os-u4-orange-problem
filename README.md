# PES-VCS: Version Control System from Scratch

## Student Details

* **Name:** Nakshathira Bharathi
* **SRN:** PESXUG24AM096

---

## Overview

This project implements a simplified version control system inspired by Git. It supports content-addressable storage, staging, commits, and history tracking.

---

## Phase 1: Object Storage

### Screenshot 1A: Test Output

<img width="631" height="209" alt="Screenshot 2026-04-17 at 8 43 50 PM" src="https://github.com/user-attachments/assets/aa34156e-eb0c-447f-ac69-73327f61add0" />
### Screenshot 1B: Object Storage Structure

<img width="626" height="103" alt="Screenshot 2026-04-17 at 8 44 02 PM" src="https://github.com/user-attachments/assets/f994f925-107b-474e-b20e-aa466a694429" />
---

## Phase 2: Tree Objects

### Screenshot 2A: Tree Test Output

<img width="629" height="149" alt="Screenshot 2026-04-17 at 8 44 15 PM" src="https://github.com/user-attachments/assets/9bd9605a-6930-48c7-aacc-42fe345ca486" />
### Screenshot 2B: Raw Tree Object (Hex Dump)

<img width="627" height="340" alt="Screenshot 2026-04-17 at 8 44 33 PM" src="https://github.com/user-attachments/assets/cdde6c77-f585-4470-bf7a-bc47046bb545" />
---

## Phase 3: Index (Staging Area)

### Screenshot 3A: pes init → add → status

<img width="632" height="266" alt="Screenshot 2026-04-17 at 8 44 51 PM" src="https://github.com/user-attachments/assets/8ba71f5b-89ac-4130-a7df-17762c4c6622" />
### Screenshot 3B: Index File Contents

<img width="642" height="125" alt="Screenshot 2026-04-17 at 8 45 11 PM" src="https://github.com/user-attachments/assets/6b41b44d-4209-4160-8fcc-f10c0b161b27" />
---

## Phase 4: Commits and History

<img width="637" height="389" alt="Screenshot 2026-04-17 at 8 45 24 PM" src="https://github.com/user-attachments/assets/67d1dac4-999d-41a8-a3bf-78eeb605a024" />
*(Paste screenshot here)*

### Screenshot 4B: Object Store Growth

<img width="630" height="222" alt="Screenshot 2026-04-17 at 8 45 41 PM" src="https://github.com/user-attachments/assets/f865ebf0-15dd-4dda-a19f-ef9db028a379" />
### Screenshot 4C: HEAD and Branch Reference

<img width="629" height="89" alt="Screenshot 2026-04-17 at 8 45 56 PM" src="https://github.com/user-attachments/assets/d29188fc-e9c2-4b0e-bfa1-22f6bfdf4c7d" />
---

## Phase 5: Branching and Checkout

### Q5.1

A checkout operation updates the HEAD to point to a new branch and updates the working directory to match the commit tree of that branch. It involves modifying `.pes/HEAD`, reading the commit’s tree, and rewriting files. This is complex because it must safely handle file overwrites and preserve consistency.

### Q5.2

A dirty working directory is detected by comparing file metadata (mtime, size, hash) between the index and working directory. If changes exist and conflict with the target branch, checkout must abort to prevent data loss.

### Q5.3

In detached HEAD state, commits are not associated with any branch and may be lost. These commits can be recovered if their hash is known or by creating a new branch pointing to them.

---

## Phase 6: Garbage Collection

### Q6.1

Garbage collection can be implemented by performing a graph traversal starting from all branch references and marking reachable objects. Any unmarked objects can be deleted. A hash set can be used to track visited objects efficiently.

### Q6.2

Running GC concurrently with commits can lead to race conditions where objects are deleted before being referenced by a new commit. Git avoids this using locking mechanisms and ensuring objects are fully written before updating references.

---

## Commands Implemented

```
pes init
pes add <file>
pes status
pes commit -m "message"
pes log
```

---

## Conclusion

This project demonstrates how version control systems use filesystem concepts such as hashing, trees, and references to efficiently track changes and maintain history.

---
