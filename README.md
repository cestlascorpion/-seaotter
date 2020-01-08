# README

基于unix socket通信的服务状态收集系统，各个client将调用结果信息发送给server，server汇总信息后定时发送给shmClient，用于持久化（未实现）。初期采用双map+mutex，然后尝试优化map，最后想了想其实只需要一个线程安全的计数器而已...233，当然这还远远不够。这是一种典型的message passing进行进程间通信的方法，消息使用protobuffer进行压缩，同机的unix socket通信则总是可靠的。

## 初始版本

client包含两个map，分别用于写入和上报。写入时调用commit()，内部为mutex；上报时采用swap的方式，交换map，内部使用同一个mutex。

## 改进

1. 沿用map+mutex的方案，优化方式有增加map数量（哈希分组）/ 构造map时指定合理的初始容量减少内存分配 / 改用其他高效的map（如google sparsepp::sparse_hash_map）；
2. 降低锁粒度。参考拉链法的哈希表数据结构，将mutex下放至哈系桶，仅提供 AddOrUpdate() + GetAndClear() 接口即可，能够有效提升性能。

### 优化锁：增加分组

申请一组mutex，将到达的commit()哈希到不同分组中。

### 更新：尝试优化map

优化a：指示map的初始的桶数量，有一定的提升；

优化b：在a基础上，更换第三方的hash map（google sparsepp::sparse_hash_map，提供与unordered map相同的接口），有一定的提升。

### 更新：去除map

map+mutex的锁颗粒度较大，因此使用canary counter。该计数器的数据结构与拉链法的hashmap相同。为了解决并发访问的问题，每个桶包含一个mutex。计数器只提供两个接口：AddOrUpdate()/GetAndClear()，分别用于单次调用信息写入和聚合上报，性能有明显的提升。
