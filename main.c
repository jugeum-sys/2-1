#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct BNode {
    char           data;
    struct BNode* left;
    struct BNode* right;
} BNode;

static const char* src;
static int  pos;
static BNode* charNode[26];    
static BNode* charParent[26];  
static int  totalAllocated = 0;

static void skipSpaces(void) {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) pos++;
}

static void fail(const char* msg) {
    fprintf(stderr, "형식 오류: %s (위치 %d)\n", msg, pos);
    exit(1);
}


static BNode* parseNode(BNode* parent) {
    skipSpaces();
    if (!isupper((unsigned char)src[pos])) return NULL;   

    char c = src[pos];
    if (charNode[c - 'A'] != NULL) fail("같은 이름의 노드가 두 번 나왔습니다");

    BNode* node = (BNode*)malloc(sizeof(BNode));
    if (!node) fail("메모리 할당 실패");
    node->data = c;
    node->left = node->right = NULL;

    charNode[c - 'A'] = node;
    charParent[c - 'A'] = parent;
    totalAllocated++;
    pos++;

    skipSpaces();
    if (src[pos] != '(') return node;                     
    pos++;                                                 

    node->left = parseNode(node);                          

    skipSpaces();
    if (src[pos] == ',') {
        pos++;                                             
        node->right = parseNode(node);                     
        skipSpaces();
    }
    if (src[pos] != ')') fail("')'가 필요합니다");
    pos++;                                                

    return node;
}


static void printSideways(BNode* node, int depth) {
    if (node == NULL) return;
    printSideways(node->right, depth + 1);
    for (int i = 0; i < depth - 1; i++) printf("    ");
    if (depth > 0) printf("+---");
    printf("%c\n", node->data);
    printSideways(node->left, depth + 1);
}


typedef struct {
    int total, leaf, nonLeaf, height, degree;
    int hasTwoChildren;
} Stats;

static void computeStats(BNode* node, int depth, Stats* st) {
    if (node == NULL) return;
    int childCount = (node->left != NULL) + (node->right != NULL);

    st->total++;
    if (childCount == 0) st->leaf++; else st->nonLeaf++;
    if (childCount > st->degree) st->degree = childCount;
    if (childCount == 2) st->hasTwoChildren = 1;
    if (depth > st->height) st->height = depth;

    computeStats(node->left, depth + 1, st);
    computeStats(node->right, depth + 1, st);
}


static int isCompleteLinked(BNode* root, int total) {
    if (root == NULL) return 1;

    int qsize = 2 * (total + 2);
    BNode** queue = (BNode**)malloc(sizeof(BNode*) * qsize);
    if (!queue) fail("메모리 할당 실패(완전성 검사용 큐)");

    int front = 0, rear = 0;
    queue[rear++] = root;
    int seenEmpty = 0;
    int result = 1;

    while (front < rear) {
        BNode* cur = queue[front++];
        if (cur == NULL) {
            seenEmpty = 1;
        }
        else {
            if (seenEmpty) { result = 0; break; }
            queue[rear++] = cur->left;
            queue[rear++] = cur->right;
        }
    }
    free(queue);
    return result;
}


int main(void) {
    static char input[4096];

    printf("===== 연결 자료구조 기반 이진트리 =====\n");
    printf("이진트리를 괄호 표기법으로 입력하세요: ");
    if (!fgets(input, sizeof(input), stdin)) return 1;
    input[strcspn(input, "\n")] = '\0';

    memset(charNode, 0, sizeof(charNode));
    memset(charParent, 0, sizeof(charParent));
    src = input; pos = 0;

    BNode* root = parseNode(NULL);
    skipSpaces();
    if (src[pos] != '\0') fail("처리되지 않은 문자가 남아 있습니다");
    if (root == NULL) { fprintf(stderr, "형식 오류: 루트 노드가 없습니다\n"); return 1; }

    printf("\n[1] 왼쪽으로 누운 이진트리\n");
    printSideways(root, 0);

    Stats st = { 0, 0, 0, 0, 0, 0 };
    computeStats(root, 1, &st);

    printf("\n[2] 트리 정보\n");
    printf("  전체 노드의 수   : %d\n", st.total);
    printf("  단말 노드의 수   : %d\n", st.leaf);
    printf("  비단말 노드의 수 : %d\n", st.nonLeaf);
    printf("  트리의 높이      : %d\n", st.height);
    printf("  트리의 차수      : %d\n", st.degree);

    int complete = isCompleteLinked(root, st.total);
    int full = complete && (st.total == (1 << st.height) - 1);
    int skewed = (st.total >= 2) && !st.hasTwoChildren;

    printf("\n[3] 이진트리 형태 판별\n");
    printf("  완전 이진트리 여부 : %s\n", complete ? "예" : "아니오");
    printf("  포화 이진트리 여부 : %s\n", full ? "예" : "아니오");
    printf("  편향 이진트리 여부 : %s\n", skewed ? "예" : "아니오");

    long nodeBytes = (long)st.total * (long)sizeof(BNode);
    printf("\n[4] 메모리 사용량 (연결 자료구조 기반)\n");
    printf("  노드 1개 크기            : %zu 바이트 (data:1 + left:%zu + right:%zu, 정렬 포함)\n",
        sizeof(BNode), sizeof(BNode*), sizeof(BNode*));
    printf("  노드 %d개 x %zu바이트     = %ld 바이트 (malloc 자체의 관리 오버헤드는 별도, 플랫폼마다 다름)\n",
        st.total, sizeof(BNode), nodeBytes);

    printf("\n[5] 노드 조회 (부모/자식/형제)\n");
    printf("조회할 노드를 입력하세요 (건너뛰려면 Enter): ");
    char line[64];
    if (fgets(line, sizeof(line), stdin) && isupper((unsigned char)line[0])) {
        char target = line[0];
        BNode* node = charNode[target - 'A'];
        BNode* parent = charParent[target - 'A'];
        if (node == NULL) {
            printf("  '%c' 노드는 트리에 없습니다\n", target);
        }
        else {
            BNode* sibling = NULL;
            if (parent != NULL) sibling = (parent->left == node) ? parent->right : parent->left;

            printf("  부모        : %s\n", parent ? (char[2]) { parent->data, 0 } : "없음(루트)");
            printf("  왼쪽 자식   : %s\n", node->left ? (char[2]) { node->left->data, 0 } : "없음");
            printf("  오른쪽 자식 : %s\n", node->right ? (char[2]) { node->right->data, 0 } : "없음");
            printf("  형제        : %s\n", sibling ? (char[2]) { sibling->data, 0 } : "없음");
            printf("  (노드 자체에는 parent 필드가 없어서, 파싱할 때 만들어 둔\n"
                "   charParent[] 보조 테이블에서 부모를 찾아왔습니다.)\n");
        }
    }

    return 0;
}
