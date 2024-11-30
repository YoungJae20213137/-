#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

typedef struct Node {
    char character;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct TextBuffer {
    Node *head;             
    Node *tail;             
    int modified;    
    char *filename;       
} TextBuffer;

typedef struct Cursor {
    Node *current;          
    int row;                
    int col;                
} Cursor;

// 노드 초기화
Node* createNode(char c) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->character = c;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

// 노드 삽입
void insertNode(TextBuffer *tb, Cursor *cursor, char c) {
    Node *newNode = createNode(c);
    tb->modified = 1;

    if (tb->head == NULL) {
        tb->head = newNode;
        tb->tail = newNode;
        cursor->current = newNode;
    } else if (cursor->current == NULL) {
        newNode->next = tb->head;
        tb->head->prev = newNode;
        tb->head = newNode;
        cursor->current = newNode;
    } else {
        newNode->prev = cursor->current;
        newNode->next = cursor->current->next;
        if (cursor->current->next != NULL) {
            cursor->current->next->prev = newNode;
        } else {
            tb->tail = newNode;
        }
        cursor->current->next = newNode;
        cursor->current = newNode;
    }
}

void deleteNode(TextBuffer *tb, Cursor *cursor) {
    if (cursor->current == NULL) {
        return;
    }
    tb->modified = 1;
    Node *nodeToDelete = cursor->current;

    if (nodeToDelete->prev != NULL) {
        nodeToDelete->prev->next = nodeToDelete->next;
        cursor->current = nodeToDelete->prev;
    } else {
        tb->head = nodeToDelete->next;
        cursor->current = tb->head;
    }

    if (nodeToDelete->next != NULL) {
        nodeToDelete->next->prev = nodeToDelete->prev;
    } else {
        tb->tail = nodeToDelete->prev;
    }

    free(nodeToDelete);
}

void modifyNode(Cursor *cursor, char c) {
    if (cursor->current != NULL) {
        cursor->current->character = c;
    }
}

void displayList(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    wclear(win);
    Node *temp = tb->head;
    int x = 0, y = 0;
    while (temp != NULL) {
        mvwaddch(win, y, x, temp->character);
        if (temp == cursor->current) {
            wmove(win, y, x);
            cursor->row = y;
            cursor->col = x;
        }
        x++;
        if (temp->character == '\n' || x >= COLS) {
            x = 0;
            y++;
        }
        temp = temp->next;
    }
    wrefresh(win);
}

// // 파일 읽기
// void load_file(TextBuffer *buffer) {
//     FILE *file = fopen(buffer->filename, "r");
//     if (!file) return; // 파일이 없으면 새 파일로 간주

//     int ch;
//     Cursor temp_cursor = { NULL, 0, 0 };
//     while ((ch = fgetc(file)) != EOF) {
//         insert_character(buffer, &temp_cursor, ch);
//     }

//     fclose(file);
// }

// // 파일 저장
// void save_file(TextBuffer *buffer) {
//     FILE *file = fopen(buffer->filename, "w");
//     if (!file) {
//         mvprintw(0, 0, "Error saving file: %s", buffer->filename);
//         refresh();
//         return;
//     }

//     Node *current = buffer->head;
//     while (current) {
//         fputc(current->character, file);
//         current = current->next;
//     }

//     fclose(file);
//     buffer->modified = 0;
// }

int main(int argc, char *argv[]) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    TextBuffer tb = {NULL, NULL, 0, NULL};
    Cursor cursor = {NULL, 0, 0};

    if (argc > 1) {
        // Load file if provided
        FILE *file = fopen(argv[1], "r");
        if (file) {
            int ch;
            while ((ch = fgetc(file)) != EOF) {
                insertNode(&tb, &cursor, (char)ch);
            }
            fclose(file);
            tb.modified = 0;
            tb.filename = strdup(argv[1]);
            cursor.current = tb.tail;
        } else {
            tb.filename = strdup(argv[1]);
        }
    }

    int ch;
    while ((ch = getch()) != KEY_F(1)) { // Press F1 to exit
        switch (ch) {
            case KEY_LEFT:
                if (cursor.current != NULL && cursor.current->prev != NULL) {
                    cursor.current = cursor.current->prev;
                }
                break;
            case KEY_RIGHT:
                if (cursor.current != NULL && cursor.current->next != NULL) {
                    cursor.current = cursor.current->next;
                }
                break;
            case KEY_UP:
                // Implement moving up if needed
                break;
            case KEY_DOWN:
                // Implement moving down if needed
                break;
            case KEY_BACKSPACE:
            case 127:
                deleteNode(&tb, &cursor);
                break;
            case KEY_DC:
                if (cursor.current != NULL && cursor.current->next != NULL) {
                    cursor.current = cursor.current->next;
                    deleteNode(&tb, &cursor);
                }
                break;
            default:
                if (ch >= 32 && ch <= 126) { // Printable characters
                    insertNode(&tb, &cursor, (char)ch);
                } else if (ch == '\n' || ch == '\r') {
                    insertNode(&tb, &cursor, '\n');
                }
                break;
        }
        displayList(stdscr, &tb, &cursor);
        move(cursor.row, cursor.col);
        refresh();
    }

    endwin();

    // Save file if modified
    if (tb.modified && tb.filename) {
        FILE *file = fopen(tb.filename, "w");
        if (file) {
            Node *temp = tb.head;
            while (temp != NULL) {
                fputc(temp->character, file);
                temp = temp->next;
            }
            fclose(file);
        }
    }

    // Free the linked list
    Node *temp;
    while (tb.head != NULL) {
        temp = tb.head;
        tb.head = tb.head->next;
        free(temp);
    }

    // Free filename if allocated
    if (tb.filename) {
        free(tb.filename);
    }

    return 0;
}