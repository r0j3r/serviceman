struct node {
    unsigned char flags;
    long deadline;
    void * data;
    struct node * c[2];
};

struct tree {
    struct node * root;
};
