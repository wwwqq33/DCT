#include <stdio.h>
#include <malloc.h>
#define MaxSize 100
typedef int KeyType;
typedef char InfoType;

typedef struct node {
	KeyType key;
	InfoType data;
	struct node *lchild, *rchild;
} BSTNode;

void DispBST(BSTNode *bt) {
	if (bt != NULL) {
		printf("%d", bt->key);
		if (bt->lchild != NULL || bt->rchild != NULL) {
			printf("(");
			DispBST(bt->lchild);
			if (bt->rchild != NULL)
				printf(",");
			DispBST(bt->rchild);
			printf(")");
		}
	}
}

bool InsertBST(BSTNode *&bt, KeyType k) {
	if (bt == NULL) {
		bt = (BSTNode *)malloc(sizeof(BSTNode));
		bt->key = k;
		bt->lchild = bt->rchild = NULL;
		return true;
	} else if (k == bt->key)
		return false;
	else if (k < bt->key)
		return InsertBST(bt->lchild, k);
	else
		return InsertBST(bt->rchild, k);
}

BSTNode *CreateBST(KeyType A[], int n) {
	BSTNode *bt = NULL;
	int i = 0;
	while (i < n) {
		if (InsertBST(bt, A[i]) == 1) {
			i++;
		}
	}
	return bt;
}


bool SearchBST(BSTNode *bt, int r) {
	while (bt != NULL) {
		if (r == bt->key) {
			printf("%d", bt->data);
			return true;
		} else if (r < bt->key)
			bt = bt->lchild;
		else
			bt = bt->rchild;
	}
	return false;
}

void Delete1(BSTNode *p, BSTNode *&r) {
	BSTNode *q;
	if (r->rchild != NULL) {
		Delete1(p, r->rchild);
	} else {
		p->key = r->key;
		p->data = r->data;
		q = r;
		r = r->lchild;
		free(q);
	}
}

void Delete(BSTNode *&p) {
	BSTNode *q;
	if (p->rchild == NULL) {
		q = p;
		p = p->lchild;
		free(p);
	} else if (p->lchild == NULL) {
		q = p;
		p = p->rchild;
		free(p);
	} else
		Delete1(p, p->lchild);
}

bool DeleteBST(BSTNode *&bt, KeyType k) {
	if (bt == NULL)
		return false;
	else {
		if (k < bt->key)
			return DeleteBST(bt->lchild, k);
		else if (k > bt->key)
			return DeleteBST(bt->rchild, k);
		else {
			Delete(bt);
			return true;
		}
	}
}

int main() {
	BSTNode *bt;
	int path[MaxSize];
	KeyType k = 2;
	int a[] = {7, 9, 0, 1, 8, 2, 3, 6, 5, 4}, n = 10;
	bt = CreateBST(a, n);
	printf("二叉排序树：");
	DispBST(bt);
	printf("\n");
	printf("查找%d关键字对应的值为：", k);
	SearchBST(bt, k);
	printf("\n");
	printf("删除结点8：");
	DeleteBST(bt, 8);
	DispBST(bt);
	printf("\n");
}