#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_LABEL 32

typedef struct Node {
    char data[MAX_LABEL];
    struct Node* left;
    struct Node* right;
} Node;

typedef struct {
    Node** items;
    int top;
    int capacity;
} Stack;

void stack_init(Stack* s) {
    s->capacity = 16;
    s->top = -1;
    s->items = (Node**)malloc(sizeof(Node*) * s->capacity);
    if (s->items == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }
}

void stack_free(Stack* s) {
    free(s->items);
    s->items = NULL;
}

int stack_empty(const Stack* s) {
    return s->top < 0;
}

void push(Stack* s, Node* n) {
    if (s->top + 1 >= s->capacity) {
        Node** tmp = (Node**)realloc(s->items, sizeof(Node*) * s->capacity * 2);
        if (tmp == NULL) {
            fprintf(stderr, "memory allocation failed\n");
            exit(1);
        }
        s->items = tmp;
        s->capacity *= 2;
    }
    s->items[++s->top] = n;
}

Node* pop(Stack* s) {
    return stack_empty(s) ? NULL : s->items[s->top--];
}

Node* peek(const Stack* s) {
    return stack_empty(s) ? NULL : s->items[s->top];
}

Node* create_node(const char* label) {
    Node* n = (Node*)calloc(1, sizeof(Node));
    if (n == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }
    strcpy(n->data, label);
    n->left = NULL;
    n->right = NULL;
    return n;
}

void free_tree(Node* root) {
    Stack s;
    if (root == NULL) return;
    stack_init(&s);
    push(&s, root);
    while (!stack_empty(&s)) {
        Node* n = pop(&s);
        if (n->left)  push(&s, n->left);
        if (n->right) push(&s, n->right);
        free(n);
    }
    stack_free(&s);
}

int count_nodes(Node* root) {
    Stack s;
    int count = 0;
    if (root == NULL) return 0;
    stack_init(&s);
    push(&s, root);
    while (!stack_empty(&s)) {
        Node* n = pop(&s);
        count++;
        if (n->left)  push(&s, n->left);
        if (n->right) push(&s, n->right);
    }
    stack_free(&s);
    return count;
}

static const char* input_start;
static const char* pos;
static int parse_error;
static char error_msg[256];

static void set_error(const char* msg) {
    if (parse_error) return;          
    parse_error = 1;
    snprintf(error_msg, sizeof(error_msg), "%s (at position %d)",
        msg, (int)(pos - input_start) + 1);
}

static void skip_spaces(void) {
    while (*pos && isspace((unsigned char)*pos)) pos++;
}

static int is_label_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static Node* parse_node(void);

static Node* parse_subtree(void) {
    skip_spaces();
    if (*pos == ',' || *pos == ')') return NULL;   
    return parse_node();
}

static Node* parse_node(void) {
    char label[MAX_LABEL];
    char msg[128];
    int len = 0;
    Node* node;

    skip_spaces();
    if (!is_label_char(*pos)) {
        if (*pos == '\0') {
            set_error("unexpected end of input, node data expected");
        }
        else {
            snprintf(msg, sizeof(msg), "node data expected but found '%c'", *pos);
            set_error(msg);
        }
        return NULL;
    }

    while (is_label_char(*pos)) {
        if (len >= MAX_LABEL - 1) {
            set_error("node data is too long");
            return NULL;
        }
        label[len++] = *pos++;
    }
    label[len] = '\0';
    node = create_node(label);

    skip_spaces();
    if (*pos != '(') return node;      
    pos++;                            

    node->left = parse_subtree();
    if (parse_error) { free_tree(node); return NULL; }
    skip_spaces();

    if (*pos == ',') {
        pos++;
        node->right = parse_subtree();
        if (parse_error) { free_tree(node); return NULL; }
        skip_spaces();
    }

    if (*pos != ')') {
        if (*pos == '\0') {
            set_error("missing ')'");
        }
        else if (*pos == ',') {
            set_error("a node in a binary tree can have at most 2 children");
        }
        else {
            snprintf(msg, sizeof(msg), "')' expected but found '%c'", *pos);
            set_error(msg);
        }
        free_tree(node);
        return NULL;
    }
    pos++;                             
    return node;
}

Node* build_tree(const char* text) {
    Node* root;

    input_start = pos = text;
    parse_error = 0;
    error_msg[0] = '\0';

    skip_spaces();
    if (*pos == '\0') {
        set_error("empty input");
        return NULL;
    }

    root = parse_node();
    if (parse_error) return NULL;

    skip_spaces();
    if (*pos != '\0') {
        if (*pos == ')') set_error("unmatched ')'");
        else             set_error("unexpected characters after the tree");
        free_tree(root);
        return NULL;
    }
    return root;
}

typedef struct {
    Node* node;
    int depth;
    int is_last;   
    char tag;     
} Frame;

void print_structure(Node* root) {
    int n, top = -1;
    Frame* st;
    int* last;

    if (root == NULL) {
        printf("(empty tree)\n");
        return;
    }
    n = count_nodes(root);
    st = (Frame*)malloc(sizeof(Frame) * (2 * n + 2));
    last = (int*)calloc(n + 2, sizeof(int));
    if (st == NULL || last == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }

    st[++top] = (Frame){ root, 0, 1, ' ' };
    while (top >= 0) {
        Frame f = st[top--];
        int i;

        last[f.depth] = f.is_last;
        for (i = 1; i < f.depth; i++)
            printf(last[i] ? "    " : "|   ");
        if (f.depth > 0)
            printf("%s%c: ", f.is_last ? "`-- " : "|-- ", f.tag);

        if (f.node == NULL) {        
            printf("-\n");
            continue;
        }
        printf("%s\n", f.node->data);

        if (f.node->left || f.node->right) {
            st[++top] = (Frame){ f.node->right, f.depth + 1, 1, 'R' };
            st[++top] = (Frame){ f.node->left,  f.depth + 1, 0, 'L' };
        }
    }
    free(st);
    free(last);
}

static void visit(Node* n, int* first) {
    printf(*first ? "%s" : " %s", n->data);
    *first = 0;
}

void preorder(Node* tree) {
    Stack s;
    int first = 1;

    if (tree == NULL) { printf("(empty)\n"); return; }
    stack_init(&s);
    push(&s, tree);
    while (!stack_empty(&s)) {
        Node* cur = pop(&s);
        visit(cur, &first);
        if (cur->right) push(&s, cur->right);
        if (cur->left)  push(&s, cur->left);
    }
    printf("\n");
    stack_free(&s);
}

void inorder(Node* tree) {
    Stack s;
    Node* cur = tree;
    int first = 1;

    if (tree == NULL) { printf("(empty)\n"); return; }
    stack_init(&s);
    while (cur != NULL || !stack_empty(&s)) {
        while (cur != NULL) {         
            push(&s, cur);
            cur = cur->left;
        }
        cur = pop(&s);
        visit(cur, &first);
        cur = cur->right;
    }
    printf("\n");
    stack_free(&s);
}

void postorder(Node* tree) {
    Stack s;
    Node* cur = tree;
    Node* last = NULL;
    int first = 1;

    if (tree == NULL) { printf("(empty)\n"); return; }
    stack_init(&s);
    while (cur != NULL || !stack_empty(&s)) {
        if (cur != NULL) {
            push(&s, cur);
            cur = cur->left;
        }
        else {
            Node* top = peek(&s);
            if (top->right != NULL && last != top->right) {
                cur = top->right;      
            }
            else {
                visit(top, &first);    
                last = pop(&s);
            }
        }
    }
    printf("\n");
    stack_free(&s);
}

int main(void) {
    char line[MAX_INPUT];

    printf("Input format example : A(B(D,E),C(,F))\n");
    printf("Empty line or 'q' to quit.\n\n");

    while (1) {
        Node* root;

        printf("Tree> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || strcmp(line, "q") == 0) break;

        printf("\nInput tree : %s\n", line);
        root = build_tree(line);
        if (root == NULL) {
            printf("[Error] Invalid tree expression: %s\n\n", error_msg);
            continue;
        }

        printf("Nodes      : %d\n\n", count_nodes(root));
        printf("[Tree structure]\n");
        print_structure(root);
        printf("\n");

        printf("Preorder  : "); preorder(root);
        printf("Inorder   : "); inorder(root);
        printf("Postorder : "); postorder(root);
        printf("\n");

        free_tree(root);
    }

    printf("Bye.\n");
    return 0;
}