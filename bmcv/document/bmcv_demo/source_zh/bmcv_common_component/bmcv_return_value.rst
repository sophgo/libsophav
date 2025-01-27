BMCV API 返回值
------------------

| 在代码中，我们定义了一个枚举类型bm_status_t，其中包含不同的状态值。

.. list-table:: bmcv返回值表
    :widths: 15 15

    * - **返回值**
      - **描述**
    * - BM_SUCCESS
      - 成功
    * - BM_ERR_FAILURE
      - 失败
    * - BM_ERR_DEVNOTREADY
      - 设备尚未准备好
    * - BM_ERR_TIMEOUT
      - 超时
    * - BM_ERR_PARAM
      - 参数无效
    * - BM_ERR_NOMEM
      - 内存不足
    * - BM_ERR_DATA
      - 数据错误
    * - BM_ERR_BUSY
      - 忙碌
    * - BM_ERR_NOFEATURE
      - 暂时不支持
    * - BM_NOT_SUPPORTED
      - 不支持




