/*
 * Для компиляции необходимо добавить ключ -lncurses
 * gcc -o snake main.c -lncurses
 Vallen-snake
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <inttypes.h>
#include <wchar.h>

enum {
    LEFT = 1, UP, RIGHT, DOWN, STOP_GAME = 'q', ROCK = 'c', SHOT = ' '
};
enum {
    MAX_TAIL_SIZE = 1000,
    START_TAIL_SIZE = 3,
    MAX_FOOD_SIZE = 20,
    MAX_ROCK_SIZE = 40,
    FOOD_EXPIRE_SECONDS = 10,
    SPEED = 20000,
    SEED_NUMBER = 3
};

enum{
    SNAKE1 = 1,
    SNAKE2 = 2,
    FOOD = 3
};

/*
 Хвост этто массив состоящий из координат x,y
 */
struct tail {
    int x;
    int y;
} tail[MAX_TAIL_SIZE], tail2[MAX_TAIL_SIZE];

/*
 Еда массив точек
 x, y - координата где установлена точка
 put_time - время когда данная точка была установлена
 point - внешний вид точки ('$','E'...)
 enable - была ли точка съедена
 */
struct food {
    int x;
    int y;
    time_t put_time;
    char point;
    uint8_t enable;
} food[MAX_FOOD_SIZE];

struct shot{
    int x;
    int y;
    int direction;
    char point;
    uint8_t enable;
} shot;

struct rock {
    int x;
    int y;
   // time_t put_time;
    char point;
    uint8_t enable;
} rock[MAX_ROCK_SIZE];

/*
 Голова змейки содержит в себе
 x,y - координаты текущей позиции
 direction - направление движения
 tsize - размер хвоста
 *tail -  ссылка на хвост
 */
struct snake {
    int number;
    int x;
    int y;
    int direction;
    size_t tsize;
    time_t eatRock;
    struct tail *tail;
    

} snake1, snake2;

void setColor(int objectType){
    attroff(COLOR_PAIR(1));
    attroff(COLOR_PAIR(2));
    attroff(COLOR_PAIR(3));
    switch (objectType){
        case 1:{ // SNAKE1
            attron(COLOR_PAIR(1));
            break;
        }
        case 2:{ // SNAKE2
            attron(COLOR_PAIR(2));
            break;
        }
        case 3:{ // FOOD
            attron(COLOR_PAIR(3));
            break;
        }
    }
}

/*
 Движение головы с учетом текущего направления движения
 */
void go(struct snake *head) {
    
    setColor(head->number);
    char ch[] = "@";
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x); // macro - размер терминала
    //clear(); // очищаем весь экран
    mvprintw(head->y, head->x, " "); // очищаем один символ
    switch (head->direction) {
        case LEFT:
            if (head->x <= 0) // Циклическое движение, что бы не
                // уходить за пределы экрана
                head->x = max_x;
            mvprintw(head->y, --(head->x), ch);
            break;
        case RIGHT:
            if (head->x >= max_x)
                head->x = 0;
            mvprintw(head->y, ++(head->x), ch);
            break;
        case UP:
            if (head->y <= 0)
                head->y = max_y;
            mvprintw(--(head->y), head->x, ch);
            break;
        case DOWN:
            if (head->y >= max_y)
                head->y = 0;
            mvprintw(++(head->y), head->x, ch);
            break;
        default:
            break;
        
    }
    refresh();

}

void magic(struct snake *snake) {
    // if ((time(NULL) - head->eatRock) < 2) {
    //         head->x = (rand()%100)+1;
    //         head->y = (rand()%100)+1;
    // }    
    if (snake->direction == RIGHT ) snake->direction = UP;
    else if (snake->direction == UP ) snake->direction = LEFT;
    else if (snake->direction == LEFT ) snake->direction = DOWN;
    else if (snake->direction == DOWN ) snake->direction = RIGHT;
}

void changeDirection(int32_t *new_direction, int32_t key) {
    switch (key) {
        case KEY_DOWN: // стрелка вниз
            *new_direction = DOWN;
            break;
        case KEY_UP: // стрелка вверх
            *new_direction = UP;
            break;
        case KEY_LEFT: // стрелка влево
            *new_direction = LEFT;
            break;
        case KEY_RIGHT: // стрелка вправо
            *new_direction = RIGHT;
            break;
        default:
            break;
    }
}


int distance(const struct snake snake, const struct food food) {   // вычисляет количество ходов до еды
    return (abs(snake.x - food.x) + abs(snake.y - food.y));
}

void autoChangeDirection(struct snake *snake, struct food food[], int foodSize) {
    int pointer = 0;
    for (int i = 1; i < foodSize; i++) {   // ищем ближайшую еду
        pointer = (distance(*snake, food[i]) < distance(*snake, food[pointer])) ? i : pointer;
    }
    if ((snake->direction == RIGHT || snake->direction == LEFT) &&
        (snake->y != food[pointer].y)) {  // горизонтальное движение
        snake->direction = (food[pointer].y > snake->y) ? DOWN : UP;
    } else if ((snake->direction == DOWN || snake->direction == UP) &&
               (snake->x != food[pointer].x)) {  // вертикальное движение
        snake->direction = (food[pointer].x > snake->x) ? RIGHT : LEFT;
    }
}


int checkDirection(int32_t dir, int32_t key) {
    if (KEY_DOWN == key && dir == UP || KEY_UP == key && dir == DOWN || KEY_LEFT == key && dir == RIGHT ||
        KEY_RIGHT == key && dir == LEFT) {
        return 0;
    } else {
        return 1;
    }
}

void initTail(struct tail t[], size_t size) {
    struct tail init_t = {0, 0};
    for (size_t i = 0; i < size; i++) {
        t[i] = init_t;
    }
}

void initHead(struct snake *head) {
    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);
    head->x = (rand()) % (max_x - 1);
    head->y = (rand()) % (max_y - 10) + 1;
    head->direction = RIGHT;
}

void initFood(struct food f[], size_t size) {
    struct food init = {0, 0, 0, 0, 0};
    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < size; i++) {
        f[i] = init;
    }
}

void init(struct snake *head, int number, struct tail *tail, size_t size) {
    clear(); // очищаем весь экран
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head);
    head->number = number;
    head->tail = tail; // прикрепляем к голове хвост
    head->tsize = size + 1;
}

/*
 Движение хвоста с учетом движения головы
 */
void goTail(struct snake *head) {
    char ch[] = "*";
    setColor(head->number);
    mvprintw(head->tail[head->tsize - 1].y, head->tail[head->tsize - 1].x, " ");
    for (size_t i = head->tsize - 1; i > 0; i--) {
        head->tail[i] = head->tail[i - 1];
        if (head->tail[i].y || head->tail[i].x)

            mvprintw(head->tail[i].y, head->tail[i].x, ch);
    }
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;
}

/*
 Увеличение хвоста на 1 элемент
 */
void addTail(struct snake *head) {
    if (head == NULL || head->tsize > MAX_TAIL_SIZE) {
        mvprintw(0, 0, "Can't add tail");
        return;
    }
    head->tsize++;
}

//Уменьшение хвоста на 1
void leftRock(struct snake *head, struct rock *fp) {
    if (head->tsize <2) {
        mvprintw(0, 1, "Can't left a rock");
        return;
    }
    head->tsize--;
    
    mvprintw(fp->y, fp->x, " ");
    fp->x = head->x;
    fp->y = head->y; 
    fp->point = 'R';
    fp->enable = 1;

}



void putBullet(struct shot *sh, struct snake * head) {
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    sh->x = head->x;
    sh->y = head->y;
    sh->direction = head->direction;
            if (sh->direction == LEFT) (sh->x)-=2;
            else if (sh->direction == UP) (sh->y)-=2;
            else if (sh->direction == RIGHT) (sh->x)+=2;
            else if (sh->direction == DOWN) (sh->y)+=2;
    sh->point = (head->direction %2) ? '-' : '|';
    sh->enable = 1;
    //setColor(FOOD);
    mvprintw(sh->y, sh->x, "-");
}

void bulletFly(struct shot * sh) {
            mvprintw(sh->y, sh->x, " ");
            char ch = sh->point;    
            if (sh->direction == LEFT) (sh->x)-=2;
            else if (sh->direction == UP) (sh->y)-=2;
            else if (sh->direction == RIGHT) (sh->x)+=2;
            else if (sh->direction == DOWN) (sh->y)+=2;
            mvprintw(sh->y, sh->x, (sh->direction %2) ? "-" : "|");    
}

_Bool haveBeat(struct snake *head, struct shot f[]) {
    for (size_t i = 0; i < MAX_FOOD_SIZE; i++)
        if (f[i].enable && head->x == f[i].x && head->y == f[i].y) {
            f[i].enable = 0;
            mvprintw(head->y+(rand()%2)-3, head->x+(rand()%3)-2, ".");
            mvprintw(head->y+(rand()%2)-3, head->x+(rand()%2)-3, ".");
            mvprintw(head->y+(rand()%3)-1, head->x+(rand()%3)-2, ".");
              
            return 1;
        }
    return 0;
}


void printHelp(char *s) {
    mvprintw(0, 0, s);
}

/*
 Обновить/разместить текущее зерно на поле
 */
void putFoodSeed(struct food *fp) {
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (max_x - 1);
    fp->y = rand() % (max_y - 2) + 1; //Не занимаем верхнюю строку
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    setColor(FOOD);
    mvprintw(fp->y, fp->x, spoint);
}




// Мигаем зерном, перед тем как оно исчезнет
void blinkFood(struct food fp[], size_t nfood) {
    time_t current_time = time(NULL);
    char spoint[2] = {0}; // как выглядит зерно '$','\0'
    for (size_t i = 0; i < nfood; i++) {
        if (fp[i].enable && (current_time - fp[i].put_time) > 6) {
            spoint[0] = (current_time % 2) ? 'S' : 's';
            setColor(FOOD);
            mvprintw(fp[i].y, fp[i].x, spoint);
        }
    }
}

void repairSeed(struct food f[], size_t nfood, struct snake *head) {
    for (size_t i = 0; i < head->tsize; i++)
        for (size_t j = 0; j < nfood; j++) {
            /* Если хвост совпадает с зерном */
            if (f[j].x == head->tail[i].x && f[j].y == head->tail[i].y && f[i].enable) {
                mvprintw(0, 0, "Repair tail seed %d", j);
                putFoodSeed(&f[j]);
            }
        }
    for (size_t i = 0; i < nfood; i++)
        for (size_t j = 0; j < nfood; j++) {
            /* Если два зерна на одной точке */
            if (i != j && f[i].enable && f[j].enable && f[j].x == f[i].x && f[j].y == f[i].y && f[i].enable) {
                mvprintw(0, 0, "Repair same seed %d", j);
                putFoodSeed(&f[j]);
            }
        }
}

/*
 Разместить еду на поле
 */
void putFood(struct food f[], size_t number_seeds) {
    for (size_t i = 0; i < number_seeds; i++) {
        putFoodSeed(&f[i]);
    }
}

void refreshFood(struct food f[], int nfood) {
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < nfood; i++) {
        if (f[i].put_time) {
            if (!f[i].enable || (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS) {
                putFoodSeed(&f[i]);
            }
        }
    }
}

_Bool haveEat(struct snake *head, struct food f[]) {
    for (size_t i = 0; i < MAX_FOOD_SIZE; i++)
        if (f[i].enable && head->x == f[i].x && head->y == f[i].y) {
            f[i].enable = 0;
            return 1;
        }
    return 0;
}

void haveEatRock(struct snake *head, struct rock r[]) {
    for (size_t i = 0; i < MAX_ROCK_SIZE; i++)
if (r[i].enable && head->x == r[i].x && head->y == r[i].y) {
    r[i].enable = 0;
    if (head->direction == 1 ) head->direction = 2;
    else if (head->direction == 2 ) head->direction = 3;
    else if (head->direction == 3 ) head->direction = 4;
    else if (head->direction == 4 ) head->direction = 1;
    //return 1;


        }
      //  return 0;
}


void printLevel(struct snake *head) {
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    if (head->number == SNAKE1){
        setColor(head->number);
        mvprintw(0, max_x - 10, "LEVEL: %d", head->tsize);
    }
    if (head->number == SNAKE2){
        setColor(head->number);
        mvprintw(1, max_x - 10, "LEVEL: %d", head->tsize);
    }
}

void printExit(struct snake *head) {
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(max_y / 2, max_x / 2 - 5, "Your LEVEL is %d", head->tsize);
}

_Bool isCrash(struct snake *head) {
    for (size_t i = 1; i < head->tsize; i++)
        if (head->x == head->tail[i].x && head->y == head->tail[i].y)
            return 1;
    return 0;
}
void startMenu()
{
    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();

    if(has_colors() == FALSE)
    {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);

    attron(COLOR_PAIR(1));
    mvprintw(1, 1, "1. Start");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(3, 1, "2. Exit");
    attron(COLOR_PAIR(1));
            mvprintw(7, 30, "@**************                              ****************@");
    attron(COLOR_PAIR(2));
            mvprintw(10, 30, "   S N A K E    S N A K E    S N A K E     S N A K E     ");
            mvprintw(13, 30, "@**************                              ****************@");

    char ch = (int) NULL;
    while(1) {
        ch = getch();
        if(ch == '1') {
            clear();
            attron(COLOR_PAIR(2));
            mvprintw(10, 50, "S N A K E ");

            attron(COLOR_PAIR(1));
            mvprintw(20, 50, "Press any key ...");
            break;
        }
        else if(ch == '2') {
            endwin();
            exit(0);
        }
    }
    refresh();
    getch();
    endwin();
}
int main() {
    //startMenu();
    //char ch[] = 'R';
    int x = 0, y = 0, key_pressed = 0;
    init(&snake1, 1, tail, START_TAIL_SIZE); //Инициализация, хвост = 3
    init(&snake2, 2, tail2, START_TAIL_SIZE); //Инициализация, хвост = 3
    initFood(food, MAX_FOOD_SIZE);
    initscr();            // Старт curses mod
    keypad(stdscr, TRUE); // Включаем F1, F2, стрелки и т.д.
    raw();                // Откдючаем line buffering
    noecho();            // Отключаем echo() режим при вызове getch
    curs_set(FALSE);    //Отключаем курсор
    printHelp("  Use arrows for control. Press 'q' for EXIT");
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    putFood(food, SEED_NUMBER);// Кладем зерна
    timeout(0);    //Отключаем таймаут после нажатия клавиши в цикле
    while (key_pressed != STOP_GAME) {
        key_pressed = getch(); // Считываем клавишу
        if (key_pressed == ROCK) { // Отложить камень
        leftRock(&snake1, rock);
        }; 

        if (key_pressed == SHOT) {
            putBullet(&shot, &snake1);
        }
        if (shot.enable) bulletFly(&shot);

        if (haveBeat(&snake2, &shot)) {
            addTail(&snake1);
            printLevel(&snake1);
            leftRock(&snake2, rock);
        }



        if(rand()%23 == 1) leftRock(&snake2, rock);
        if (checkDirection(snake1.direction, key_pressed)) //Проверка корректности смены направления
        {
            changeDirection(&snake1.direction, key_pressed); // Меняем напарвление движения
        }
        autoChangeDirection(&snake2, food, SEED_NUMBER);
        if (isCrash(&snake1))
            break;



        go(&snake1); // Рисуем новую голову
        goTail(&snake1); //Рисуем хвост

        go(&snake2); // Рисуем новую голову
        goTail(&snake2); //Рисуем хвост
      

        if (haveEat(&snake1, food)) {
            addTail(&snake1);
            printLevel(&snake1);
        }
        if (haveEat(&snake2, food)) {
            addTail(&snake2);
            printLevel(&snake2);
        }




        refreshFood(food, SEED_NUMBER);// Обновляем еду
        repairSeed(food, SEED_NUMBER, &snake1);
        blinkFood(food, SEED_NUMBER);
        timeout(100); // Задержка при отрисовке
    }
    //setColor(SNAKE1);
    printExit(&snake1);
    timeout(SPEED);
    getch();
    endwin(); // Завершаем режим curses mod

    return 0;
}
