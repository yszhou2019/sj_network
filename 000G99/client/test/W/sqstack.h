/* P.10 ��Ԥ���峣�������� */
#define TRUE		1
#define FALSE		0
#define OK		1
#define ERROR		0
#define INFEASIBLE	-1
#define LOVERFLOW	-2	//������<math.h>�еĶ����ͻ

typedef int Status;

/* P.46 �ṹ�嶨�� */
#define STACK_INIT_SIZE	100	//��ʼ��С����Ϊ100���ɰ����޸ģ�
#define STACKINCREMENT	10	//���ռ䲻����ÿ������10���ɰ����޸ģ�

typedef int SElemType;	//�ɸ�����Ҫ�޸�Ԫ�ص�����

typedef struct {
    SElemType *base;	//��Ŷ�̬����ռ���׵�ַ
    SElemType *top;     //ջ��ָ��
    int stacksize;	//��ǰ�����Ԫ�صĸ���
} SqStack;

/* P.46-47�ĳ����������Ͷ���ת��Ϊʵ�ʵ�C���� */
Status	InitStack(SqStack *S);
Status	DestroyStack(SqStack *S);
Status	ClearStack(SqStack *S);
Status	StackEmpty(SqStack S);
int	StackLength(SqStack S);
Status	GetTop(SqStack S, SElemType *e);
Status	Push(SqStack *S, SElemType e);
Status	Pop(SqStack *S, SElemType *e);
Status	StackTraverse(SqStack S, Status (*visit)(SElemType e));
