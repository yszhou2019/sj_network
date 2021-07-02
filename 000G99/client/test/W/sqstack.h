/* P.10 的预定义常量和类型 */
#define TRUE		1
#define FALSE		0
#define OK		1
#define ERROR		0
#define INFEASIBLE	-1
#define LOVERFLOW	-2	//避免与<math.h>中的定义冲突

typedef int Status;

/* P.46 结构体定义 */
#define STACK_INIT_SIZE	100	//初始大小定义为100（可按需修改）
#define STACKINCREMENT	10	//若空间不够，每次增长10（可按需修改）

typedef int SElemType;	//可根据需要修改元素的类型

typedef struct {
    SElemType *base;	//存放动态申请空间的首地址
    SElemType *top;     //栈顶指针
    int stacksize;	//当前分配的元素的个数
} SqStack;

/* P.46-47的抽象数据类型定义转换为实际的C语言 */
Status	InitStack(SqStack *S);
Status	DestroyStack(SqStack *S);
Status	ClearStack(SqStack *S);
Status	StackEmpty(SqStack S);
int	StackLength(SqStack S);
Status	GetTop(SqStack S, SElemType *e);
Status	Push(SqStack *S, SElemType e);
Status	Pop(SqStack *S, SElemType *e);
Status	StackTraverse(SqStack S, Status (*visit)(SElemType e));
