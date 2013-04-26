
void *wr_pool_init(
    void *(*get) (int),
    void *(*remove) (int),
    void (*insert) (void *, int),
    void *dudItem
);

void wr_pool_release(void *pool); 

int wr_obtain(void *pool, int num);

void wr_release(void *pool, int *num);

void *wr_get(void *pool, int *num);

int wr_get_num_references(void *pool, int num);

void wr_remove(void *pool, int num);
