#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct Token Token;
typedef struct Node Node;

typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,     // 数値
  TK_EOF,     // EOF
} TokenKind;

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_LT, // <
  ND_LE, // <=
  ND_EQ, // ==
  ND_NE, // !=
  ND_NUM, // 整数
} NodeKind;

void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool at_eof();
Token *tokenize(char *p);
void error_at(char* loc, char* fmt, ...);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *primary();
Node *mul();
Node *relational();
Node *equality();
Node *add();
Node *unary();
Node *expr();
void gen(Node *node);

struct Node {
  NodeKind kind;
  Node* lhs;
  Node* rhs;
  int val;
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->val = -1;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *equality() {
  Node *node = relational();
  for(;;) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();
  for(;;) {
    if (consume("<")) {
      node = new_node(ND_LT, node, relational());
    } else if (consume(">")) {
      // greater than は左右を入れ替えるだけ
      node = new_node(ND_LT, relational(), node);
    } else if (consume("<=")) {
      node = new_node(ND_LE, node, relational());
    } else if (consume(">=")) {
      // greater than は左右を入れ替えるだけ
      node = new_node(ND_LE, relational(), node);
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();
  for (;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

Node *unary() {
  if (consume("+")) {
    return primary();
  } else if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }
  return new_node_num(expect_number());
}

Node *expr() {
  return equality();
}

void gen(Node *node) {
  if (node == NULL) {
    error("null pointer exeption at node(%d)\n");
  }
  // fprintf(stderr, "gen(node=%p, kind=%d, lhs=%p, rhs=%p, val=%d)\n", node, node->kind, node->lhs, node->rhs, node->val);
  if (node->kind == ND_NUM) {
    printf ("  push %d\n", node->val);
    return;
  }

  if (node->lhs == NULL) {
    error("null pointer exeption at lhs(%d)\n", node->kind);
  }
  gen(node->lhs);
  
  if (node->rhs == NULL) {
    error("null pointer exeption at rhs(%d)\n", node->kind);
  }
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch(node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;  
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;  
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}

struct Token {
  TokenKind kind;   // トークンの型
  Token *next;      // 次の入力トークン
  int val;          // kind が TK_NUM の場合、その数値
  char *str;        // トークン文字列
  int len;          // トークン文字列の長さ
};

Token* token;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号かどうかを返す
bool consume(char *op) {
  if (token->kind != TK_RESERVED || 
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    return false;
  }
  token = token->next;

  return true;
}

// 次のトークンが期待している記号のときはトークンを読み進める。
void expect(char* op) {
  if (token->kind != TK_RESERVED || strncmp(op, token->str, token->len)) {
    error_at(token->str, "'%c'ではありません", op);
  }
  token = token->next;
}

// 次のトークンが数値のときはトークンを読み進める。
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "数値ではありません");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while(*p) {
    if (isspace(*p)) {
      ++p;
      continue;
    }

    if (strncmp(p, "!=", 2) == 0 ||
        strncmp(p, "==", 2) == 0 || 
        strncmp(p, ">=", 2) == 0 ||
        strncmp(p, "<=", 2) == 0 
       ) {
        cur = new_token(TK_RESERVED, cur, p++, 2);
        p += 2;
       }

    if (*p == '+' ||
        *p == '-' ||
        *p == '*' ||
        *p == '/' ||
        *p == '(' ||
        *p == ')' ||
        *p == '>' ||
        *p == '<') {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "予期しない文字が現れました");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

char *user_input;

void error_at(char* loc, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません。");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  
  return 0;
}
