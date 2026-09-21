#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_NODES (1 << 16)   
#define MAX_INPUT 4096

static char tree[MAX_NODES];       
static int  charIndex[26];         
static const char* src;
static int  pos;
static int  maxIndexUsed = 0;      


static void skipSpaces(void) {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) pos++;
}

static void fail(const char* msg) {
    fprintf(stderr, "형식 오류: %s (위치 %d)\n", msg, pos);
    exit(1);
}

static void parseNode(int idx) {
    skipSpaces();
    if (!isupper((unsigned char)src[pos])) return;   

    if (idx <= 0 || idx >= MAX_NODES) fail("트리가 너무 깊습니다(배열 크기 초과)");

    char c = src[pos];
    if (charIndex[c - 'A'] != 0) fail("같은 이름의 노드가 두 번 나왔습니다");

    tree[idx] = c;
    charIndex[c - 'A'] = idx;
    if (idx > maxIndexUsed) maxIndexUsed = idx;
    pos++;

    skipSpaces();
    if (src[pos] != '(') return;                      
    pos++;                                            

    parseNode(idx * 2);                               

    skipSpaces();
    if (src[pos] == ',') {
        pos++;                                        
        parseNode(idx * 2 + 1);                       
        skipSpaces();
    }
    if (src[pos] != ')') fail("')'가 필요합니다");
    pos++;                                            
}


static void printSideways(int idx, int depth) {
    if (idx >= MAX_NODES || tree[idx] == 0) return;
    printSideways(idx * 2 + 1, depth + 1);            
    for (int i = 0; i < depth - 1; i++) printf("    ");
    if (depth > 0) printf("+---");
    printf("%c\n", tree[idx]);
    printSideways(idx * 2, depth + 1);                 
}



typedef struct {
    int total, leaf, nonLeaf, height, degree;
    int hasTwoChildren;
} Stats;

static void computeStats(int idx, int depth, Stats* st) {
    if (idx >= MAX_NODES || tree[idx] == 0) return;

    int hasLeft = (idx * 2 < MAX_NODES) && tree[idx * 2] != 0;
    int hasRight = (idx * 2 + 1 < MAX_NODES) && tree[idx * 2 + 1] != 0;
    int childCount = hasLeft + hasRight;

    st->total++;
    if (childCount == 0) st->leaf++; else st->nonLeaf++;
    if (childCount > st->degree) st->degree = childCount;
    if (childCount == 2) st->hasTwoChildren = 1;
    if (depth > st->height) st->height = depth;

    computeStats(idx * 2, depth + 1, st);
    computeStats(idx * 2 + 1, depth + 1, st);
}


static int isCompleteArray(int total) {
    for (int i = 1; i <= total; i++) {
        if (tree[i] == 0) return 0;
    }
    return 1;
}



int main(void) {
    static char input[MAX_INPUT];

    printf("===== 배열 기반 이진트리 =====\n");
    printf("이진트리를 괄호 표기법으로 입력하세요: ");
    if (!fgets(input, sizeof(input), stdin)) return 1;
    input[strcspn(input, "\n")] = '\0';

    memset(tree, 0, sizeof(tree));
    memset(charIndex, 0, sizeof(charIndex));
    src = input; pos = 0;

    parseNode(1);
    skipSpaces();
    if (src[pos] != '\0') fail("처리되지 않은 문자가 남아 있습니다");
    if (tree[1] == 0) { fprintf(stderr, "형식 오류: 루트 노드가 없습니다\n"); return 1; }

    
    printf("\n[1] 왼쪽으로 누운 이진트리\n");
    printSideways(1, 0);

    
    Stats st = { 0, 0, 0, 0, 0, 0 };
    computeStats(1, 1, &st);

    printf("\n[2] 트리 정보\n");
    printf("  전체 노드의 수   : %d\n", st.total);
    printf("  단말 노드의 수   : %d\n", st.leaf);
    printf("  비단말 노드의 수 : %d\n", st.nonLeaf);
    printf("  트리의 높이      : %d\n", st.height);
    printf("  트리의 차수      : %d\n", st.degree);

    
    int complete = isCompleteArray(st.total);
    int full = complete && (st.total == (1 << st.height) - 1);
    int skewed = (st.total >= 2) && !st.hasTwoChildren;

    printf("\n[3] 이진트리 형태 판별\n");
    printf("  완전 이진트리 여부 : %s\n", complete ? "예" : "아니오");
    printf("  포화 이진트리 여부 : %s\n", full ? "예" : "아니오");
    printf("  편향 이진트리 여부 : %s\n", skewed ? "예" : "아니오");

    
    long allocatedBytes = (long)MAX_NODES * (long)sizeof(char);
    long usedSlots = maxIndexUsed;              
    long usedBytes = usedSlots * (long)sizeof(char);
    long payloadBytes = (long)st.total * (long)sizeof(char);

    printf("\n[4] 메모리 사용량 (배열 기반)\n");
    printf("  배열 전체 크기            : %ld 바이트 (칸 %d개 x %zu바이트)\n",
        allocatedBytes, MAX_NODES, sizeof(char));
    printf("  실제 사용된 인덱스 범위   : 1 ~ %d (칸 %ld개, %ld바이트)\n",
        maxIndexUsed, usedSlots, usedBytes);
    printf("  그 중 데이터가 있는 노드  : %d개 (%ld바이트), 낭비된 칸 : %ld개 (%ld바이트)\n",
        st.total, payloadBytes, usedSlots - st.total, usedBytes - payloadBytes);

    printf("\n[5] 노드 조회 (부모/자식/형제)\n");
    printf("조회할 노드를 입력하세요 (건너뛰려면 Enter): ");
    char line[64];
    if (fgets(line, sizeof(line), stdin) && isupper((unsigned char)line[0])) {
        char target = line[0];
        int idx = charIndex[target - 'A'];
        if (idx == 0) {
            printf("  '%c' 노드는 트리에 없습니다\n", target);
        }
        else {
            int parentIdx = idx / 2;
            int leftIdx = idx * 2;
            int rightIdx = idx * 2 + 1;
            int siblingIdx = (idx == 1) ? 0 : (idx % 2 == 0 ? idx + 1 : idx - 1);

            printf("  부모   : %s\n", (idx > 1) ? (char[2]) { tree[parentIdx], 0 } : "없음(루트)");
            printf("  왼쪽 자식 : %s\n", (leftIdx < MAX_NODES && tree[leftIdx] != 0) ? (char[2]) { tree[leftIdx], 0 } : "없음");
            printf("  오른쪽 자식 : %s\n", (rightIdx < MAX_NODES && tree[rightIdx] != 0) ? (char[2]) { tree[rightIdx], 0 } : "없음");
            printf("  형제   : %s\n", (siblingIdx > 0 && tree[siblingIdx] != 0) ? (char[2]) { tree[siblingIdx], 0 } : "없음");
            printf("  (인덱스 %d를 알고 나면 부모=idx/2, 왼쪽=idx*2, 오른쪽=idx*2+1, 형제=idx^1 산술 연산만으로 O(1) 계산)\n", idx);
        }
    }

    return 0;
}