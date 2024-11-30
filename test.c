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

Node* createNode(char c) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->character = c;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

void insertNode(Node **head, Node **tail, Node *current, char c) {
    Node *newNode = createNode(c);
    if (*head == NULL) {
        // Empty list
        *head = newNode;
        *tail = newNode;
    } else if (current == NULL) {
        // Insert at the beginning
        newNode->next = *head;
        (*head)->prev = newNode;
        *head = newNode;
    } else {
        // Insert after current
        newNode->prev = current;
        newNode->next = current->next;
        if (current->next != NULL) {
            current->next->prev = newNode;
        } else {
            // Current is tail
            *tail = newNode;
        }
        current->next = newNode;
    }
}

void deleteNode(Node **head, Node **tail, Node **current) {
    if (*current == NULL) {
        return;
    }
    Node *nodeToDelete = *current;

    if (nodeToDelete->prev != NULL) {
        nodeToDelete->prev->next = nodeToDelete->next;
    } else {
        // Deleting head
        *head = nodeToDelete->next;
    }

    if (nodeToDelete->next != NULL) {
        nodeToDelete->next->prev = nodeToDelete->prev;
        *current = nodeToDelete->next;
    } else {
        // Deleting tail
        *tail = nodeToDelete->prev;
        *current = nodeToDelete->prev;
    }

    free(nodeToDelete);
}

void modifyNode(Node *current, char c) {
    if (current != NULL) {
        current->character = c;
    }
}

void displayList(WINDOW *win, Node *head, Node *current) {
    wclear(win);
    Node *temp = head;
    int x = 0, y = 0;
    while (temp != NULL) {
        mvwaddch(win, y, x, temp->character);
        if (temp == current) {
            mvwchgat(win, y, x, 1, A_REVERSE, 0, NULL);
        }
        x++;
        if (x >= COLS) {
            x = 0;
            y++;
        }
        temp = temp->next;
    }
    wrefresh(win);
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    Node *head = NULL;
    Node *tail = NULL;
    Node *current = NULL;

    int ch;
    while ((ch = getch()) != KEY_F(1)) { // Press F1 to exit
        switch (ch) {
            case KEY_LEFT:
                if (current != NULL && current->prev != NULL) {
                    current = current->prev;
                }
                break;
            case KEY_RIGHT:
                if (current != NULL && current->next != NULL) {
                    current = current->next;
                }
                break;
            case KEY_BACKSPACE:
            case 127:
                if (current != NULL) {
                    deleteNode(&head, &tail, &current);
                }
                break;
            case KEY_DC:
                if (current != NULL && current->next != NULL) {
                    Node *temp = current->next;
                    deleteNode(&head, &tail, &temp);
                }
                break;
            default:
                if (ch >= 32 && ch <= 126) { // Printable characters
                    insertNode(&head, &tail, current, (char)ch);
                    if (current == NULL) {
                        current = head;
                    } else {
                        current = current->next;
                    }
                }
                break;
        }
        displayList(stdscr, head, current);
    }

    endwin();

    // Free the linked list
    Node *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }

    return 0;
}