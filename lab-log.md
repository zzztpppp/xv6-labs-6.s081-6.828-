# Memory allocator
* How can I give each lock a different name without malloc?
  * Maintain a global static `char[]` that is big engouth to hold 
    names for every lock
* Implement a per-cpu freelist for each cpu is rather straightforward.

# Buffer cache
* Implementing a hash table with locks for each slot.
* Moving the global lock around different sections to test whether a certain section is
critical is useful before we fully understand how file system works.
  
* How can I make searching for cache and re-allocating unused cache atomic? If not, one thread
exiting searching for cache with `blockno` might leave another thread with the same `blockno` entering
  the eviction procedure and evicts again resulting in one block with two caches.
  * If we don't find the cache, release the slot lock and acquire all locks to serialize eviction. Before
  we start to evict, we need to scan the slot again. Since we have to make sure that no other threads have cached 
    the block for us after we released the slot lock but before we acquired all the locks.