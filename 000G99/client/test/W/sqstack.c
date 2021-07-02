#include <stdio.h>
#include <stdlib.h>		//malloc/realloc����
//#include <unistd.h>		//exit����
#include "sqstack.h"	//��ʽ����

/* ��ʼ��ջ */
Status InitStack(SqStack *S)
{
    S->base = (SElemType *)malloc(STACK_INIT_SIZE * sizeof(SElemType));
    if (S->base == NULL)
    	exit(LOVERFLOW);
    S->top = S->base;  //ջ��ָ��ָ��ջ�ף���ʾջ��
    S->stacksize = STACK_INIT_SIZE;
    return OK;
}

/* ɾ��ջ */
Status DestroyStack(SqStack *S)
{
    /* ��δִ�� InitStack��ֱ��ִ�б�����������ܳ�����Ϊָ���ʼֵδ�� */
    if (S->base)
    	free(S->base);  //Ҫ���ǿ�ջɾ������������Ҫ�ж�
    S->top = NULL;
    S->stacksize = 0;

    return OK;
}

/* ���ջ���ѳ�ʼ�������ͷſռ䣬ֻ������ݣ� */
Status ClearStack(SqStack *S)
{
    /* ���ջ������չ�����ָ���ʼ�Ĵ�С */
    if (S->stacksize > STACK_INIT_SIZE) {
    	S->base = (SElemType *)realloc(S->base, STACK_INIT_SIZE * sizeof(SElemType));
    	if (S->base==NULL)
    	    exit(LOVERFLOW);
	S->stacksize = STACK_INIT_SIZE;
	}

    S->top = S->base; //ջ��ָ��ָ��ջ������ʾջ��
    return OK;
}

/* �ж��Ƿ�Ϊ��ջ */
Status StackEmpty(SqStack S)
{
    if (S.top == S.base)
    	return TRUE;
    else
    	return FALSE;
}

/* ��ջ�ĳ��� */
int StackLength(SqStack S)
{
    return S.top - S.base;//ָ�������ֵΪ����Ԫ�ظ���
}

/* ȡջ��Ԫ�� */
Status GetTop(SqStack S, SElemType *e)
{
    if (S.top == S.base)
    	return ERROR;

    *e = *(S.top-1);	//�±��0��ʼ��top��ʵ��ջ��+1
    return OK;
}

/* Ԫ����ջ */
Status Push(SqStack *S, SElemType e)
{
    /* ���ջ������������ռ� */
    if (S->top - S->base >= S->stacksize) {
    	S->base = (SElemType *)realloc(S->base, (S->stacksize+STACKINCREMENT)*sizeof(SElemType));
    	if (S->base==NULL)
    	    return LOVERFLOW;
    	S->top = S->base + S->stacksize; //��ΪS->base���ܻ�䣬���Ҫ����S->top��ֵ
    	S->stacksize += STACKINCREMENT;
	}
    *S->top++ = e;  //��*(S->top)����S->top++
    return OK;
}

/* Ԫ�س�ջ */
Status Pop(SqStack *S, SElemType *e)
{
    int length;

    if (S->top == S->base)
    	return ERROR;
    *e = *--S->top;

    /* ���ջ��С������С��̬����ռ�Ĵ�С */
    length = S->top - S->base;
    if (S->stacksize > STACK_INIT_SIZE && S->stacksize - length >= STACKINCREMENT) {
    	S->base = (SElemType *)realloc(S->base, (S->stacksize-STACKINCREMENT)*sizeof(SElemType));
    	if (S->base==NULL)
    	    return LOVERFLOW;
    	S->top = S->base + length; //��S->base�仯��������S->top��ֵ
    	S->stacksize -= STACKINCREMENT;
	}

    return OK;
}

/* ����ջ */
Status StackTraverse(SqStack S, Status (*visit)(SElemType e))
{
    extern int line_count; //��main�ж���Ĵ�ӡ���м����������㷨�޹أ�
    SElemType *t = S.base;

    line_count = 0;		//�������ָ���ʼֵ�����㷨�޹أ�
    while(t<S.top && (*visit)(*t)==TRUE)
        t++;

    if (t<S.top)
    	return ERROR;

    printf("\n");//����ӡһ�����У�ֻ��Ϊ�˺ÿ������㷨�޹�
    return OK;
}
