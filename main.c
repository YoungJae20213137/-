/*========================== 컴파일 방법 =============================================================================================================================================================================

1) 링커 위치
ncurses: c/Program Files/gcctemp/project/etc/PDCurses-master/curses.h
pdcurses: c/Program Files/gcctemp/project/etc/PDCurses-master/wincon/pdcurses.a

2) 콘솔 모드로 컴파일
gcc -o my_program main.c -I/c/Program\ Files/gcctemp/project/etc/PDCurses-master -L/c/Program\ Files/gcctemp/project/etc/PDCurses-master/wincon -l:pdcurses.a

3) gui 모드로 컴파일
gcc -o my_program main.c -I/c/Program\ Files/gcctemp/project/etc/PDCursesMod-master -L/c/Program\ Files/gcctemp/project/etc/PDCursesMod-master/wingui -l:libpdcurses.a -lwinmm -lgdi32 -lcomdlg32

4) MAC에서 컴파일
gcc -o my_program main.c -lncurses
===================================================================================================================================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>


#define MAX_LINES 1024
#define MAX_LINE_LENGTH 256

// 자료구조 정의
typedef struct Node {
    char character;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct TextBuffer {
    Node *head;             // 리스트의 첫 번째 노드
    Node *tail;             // 리스트의 마지막 노드
    Node *cursor;           // 현재 커서 위치
    int modified;           // 파일이 수정되었는지를 나타내는 플래그
    char filename[256];     // 파일 이름 저장
    int num_lines;          // 총 줄 수
    char *lines[MAX_LINES]; // 각 줄 관리 배열
    int cursor_row;         // 커서 위치 (행)
    int cursor_col;         // 커서 위치 (열)
} TextBuffer;

// 노드 생성 함수
Node* create_node(char character) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->character = character;
    new_node->prev = NULL;
    new_node->next = NULL;
    return new_node;
}

// 텍스트 버퍼 생성 함수
TextBuffer* create_text_buffer(const char *filename) {
    TextBuffer *buffer = (TextBuffer*)malloc(sizeof(TextBuffer));
    buffer->head = NULL;
    buffer->tail = NULL;
    buffer->cursor = NULL;
    buffer->modified = 0;
    buffer->num_lines = 1;
    buffer->cursor_row = 0;
    buffer->cursor_col = 0;

    strncpy(buffer->filename, filename ? filename : "[No Name]", sizeof(buffer->filename) - 1);
    buffer->filename[sizeof(buffer->filename) - 1] = '\0';

    for (int i = 0; i < MAX_LINES; i++) {
        buffer->lines[i] = NULL;
    }
    buffer->lines[0] = (char*)calloc(MAX_LINE_LENGTH, sizeof(char));

    return buffer;
}

// 상태 바 업데이트 함수
void update_status_bar(TextBuffer *buffer) {
    char status[256];
    snprintf(status, sizeof(status), "%s%s - %d lines",
             buffer->filename,
             buffer->modified ? " (modified)" : "",
             buffer->num_lines);

    char cursor_position[32];
    snprintf(cursor_position, sizeof(cursor_position), "Cursor: (%d, %d)",
             buffer->cursor_row + 1, buffer->cursor_col + 1);

    move(LINES - 3, 0);
    attron(A_REVERSE);
    clrtoeol();
    printw("%s", status);

    int cursor_position_col = COLS - strlen(cursor_position);
    mvprintw(LINES - 3, cursor_position_col, "%s", cursor_position);
    attroff(A_REVERSE);
    refresh();
}

// 메시지 바 업데이트 함수
void update_message_bar(const char *message) {
    move(LINES - 1, 0);
    attron(A_REVERSE);
    clrtoeol();
    printw("%s", message ? message : "Save: Ctrl+S | Search: Ctrl+F | Exit: Ctrl+Q");
    attroff(A_REVERSE);
    refresh();
}

// 문자 삽입 함수
void insert_character(TextBuffer *buffer, char character) {
    char *line = buffer->lines[buffer->cursor_row];
    int len = strlen(line);

    if (len < MAX_LINE_LENGTH - 1) {
        for (int i = len; i > buffer->cursor_col; i--) {
            line[i] = line[i - 1];
        }
        line[buffer->cursor_col] = character;
        line[len + 1] = '\0';

        buffer->cursor_col++;
        mvaddch(buffer->cursor_row, buffer->cursor_col - 1, character);
        refresh();
    }
}

// 백스페이스 처리 함수
void delete_character(TextBuffer *buffer) {
    if (buffer->cursor_col > 0) {
        char *line = buffer->lines[buffer->cursor_row];
        int len = strlen(line);

        for (int i = buffer->cursor_col - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        buffer->cursor_col--;
        move(buffer->cursor_row, buffer->cursor_col);
        clrtoeol();
        mvprintw(buffer->cursor_row, 0, "%s", line);
        refresh();
    } else if (buffer->cursor_row > 0) {
        // 현재 줄 삭제 및 이전 줄 병합
        char *current_line = buffer->lines[buffer->cursor_row];
        char *prev_line = buffer->lines[buffer->cursor_row - 1];
        int prev_len = strlen(prev_line);

        if (prev_len + strlen(current_line) < MAX_LINE_LENGTH) {
            strcat(prev_line, current_line);
            free(current_line);

            for (int i = buffer->cursor_row; i < buffer->num_lines - 1; i++) {
                buffer->lines[i] = buffer->lines[i + 1];
            }
            buffer->lines[buffer->num_lines - 1] = NULL;
            buffer->num_lines--;

            buffer->cursor_row--;
            buffer->cursor_col = prev_len;

            clear();
            for (int i = 0; i < buffer->num_lines; i++) {
                mvprintw(i, 0, "%s", buffer->lines[i]);
            }
            refresh();
        }
    }
}

// 엔터 처리 함수
void insert_newline(TextBuffer *buffer) {
    if (buffer->num_lines < MAX_LINES) {
        char *line = buffer->lines[buffer->cursor_row];
        char *new_line = (char*)calloc(MAX_LINE_LENGTH, sizeof(char));

        strcpy(new_line, &line[buffer->cursor_col]);
        line[buffer->cursor_col] = '\0';

        for (int i = buffer->num_lines; i > buffer->cursor_row + 1; i--) {
            buffer->lines[i] = buffer->lines[i - 1];
        }
        buffer->lines[buffer->cursor_row + 1] = new_line;
        buffer->num_lines++;

        buffer->cursor_row++;
        buffer->cursor_col = 0;

        clear();
        for (int i = 0; i < buffer->num_lines; i++) {
            mvprintw(i, 0, "%s", buffer->lines[i]);
        }
        refresh();
    }
}

// main 함수
int main(int argc, char *argv[]) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);

    TextBuffer *buffer = create_text_buffer((argc > 1) ? argv[1] : "[No Name]");
    update_status_bar(buffer);
    update_message_bar(NULL);

    int ch;
    while ((ch = getch()) != 17) {  // Ctrl+Q 종료
        if (ch == KEY_BACKSPACE) {
            delete_character(buffer);
        } else if (ch == '\n') {
            insert_newline(buffer);
        } else if (isprint(ch)) {
            insert_character(buffer, ch);
        }

        update_status_bar(buffer);

    }

    for (int i = 0; i < buffer->num_lines; i++) {
        free(buffer->lines[i]);
    }
    free(buffer);

    endwin();
    return 0;
}
