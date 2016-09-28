typedef struct Proc Proc;
void *emalloc(int n);
Proc *new_item (pid_t pid, char * line);
Proc *add_front(Proc *listp, Proc *newp);
Proc *delete_item (Proc *listp, pid_t pid);
void update_isStop(Proc *listp, pid_t pid, int val);
int lookup_pid(Proc *listp, pid_t pid);
void print_all(Proc *listp);
void free_all(Proc *listp);
