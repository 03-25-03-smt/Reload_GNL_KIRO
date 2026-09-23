# get_next_line

Проект 42 School. Функция читает из файлового дескриптора **одну строку за вызов**,
включая завершающий `\n`, и возвращает `NULL`, когда читать больше нечего или
произошла ошибка.

```c
char *get_next_line(int fd);
```

| | |
|---|---|
| Разрешённые функции | `read`, `malloc`, `free` |
| Флаги компиляции | `-Wall -Wextra -Werror` |
| Размер буфера | задаётся при компиляции: `-D BUFFER_SIZE=n` |
| Norminette | v3 — `OK!` на всех шести файлах |

## Файлы

### Mandatory
| Файл | Назначение |
|---|---|
| `get_next_line.h` | прототипы, `BUFFER_SIZE` по умолчанию 42 |
| `get_next_line.c` | `get_next_line` + хелперы |
| `get_next_line_utils.c` | `ft_strchr`, `ft_strlen`, `ft_strdup`, `ft_strlcat`, `ft_strjoin` |

### Bonus
| Файл | Назначение |
|---|---|
| `get_next_line_bonus.h` | то же + `<sys/select.h>` для `FD_SETSIZE`, `BUFFER_SIZE` 1024 |
| `get_next_line_bonus.c` | независимое состояние на каждый fd |
| `get_next_line_utils_bonus.c` | те же утилиты |

Mandatory и bonus — **независимые сборки**. Не компилируй их вместе: обе части
определяют символ `get_next_line`.

Каждый `.c` включает **свой** хедер. Если в `get_next_line_utils_bonus.c`
оставить `#include "get_next_line.h"`, локально соберётся (оба хедера лежат
рядом), а на проверке, где bonus компилируют только из bonus-файлов, упадёт
с `fatal error: No such file or directory`.

## Сборка

```bash
# mandatory
cc -Wall -Wextra -Werror -D BUFFER_SIZE=42 \
   get_next_line.c get_next_line_utils.c main.c -o gnl

# bonus
cc -Wall -Wextra -Werror -D BUFFER_SIZE=42 \
   get_next_line_bonus.c get_next_line_utils_bonus.c main.c -o gnl_bonus
```

## Пример

```c
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
	int		fd;
	char	*line;

	fd = open("file.txt", O_RDONLY);
	while ((line = get_next_line(fd)))
	{
		printf("%s", line);
		free(line);   /* вызывающий владеет строкой */
	}
	close(fd);
	return (0);
}
```

## Как это работает

Состояние живёт в статическом `stash` — это уже прочитанные байты, которые
ещё не отдали наружу.

```
   get_next_line(fd)
        │
   stash == NULL ──► ft_strdup("")   создаём пустую строку
        │
        ▼
   в stash есть '\n'? ──нет──► fill_stash()
        │                        read() до BUFFER_SIZE байт
        │                        stash = strjoin(stash, buf)
        │                        вернул NULL (EOF/ошибка) ──► цикл выходит
        │да
        ▼
   fill_line(stash)  ──► копия до '\n' включительно  ──► return
   stash_cut(stash)  ──► новый stash = хвост после '\n'
```

| Функция | Что делает |
|---|---|
| `fill_stash` | один `read`, приклеивает результат к `stash`; при ошибке чтения обнуляет `stash` |
| `fill_line` | выделяет и возвращает строку до `'\n'` включительно |
| `stash_cut` | возвращает новый `stash` — всё, что осталось после `'\n'` |
| `free_stash` | освобождает `stash`, ставит `NULL`, возвращает `NULL` |
| `get_next_line` | связывает всё вместе |

Пять функций на файл — норма разрешает ровно столько. `fill_stash`,
`fill_line` и `stash_cut` объявлены `static`.

Bonus отличается одним: `stash` — это массив `char *stash[FD_SETSIZE]`, и вся
работа идёт через `stash[fd]`. Это по-прежнему **одна** статическая переменная.

## Контракт

| Вход | Результат |
|---|---|
| строка с `\n` | строка вместе с `\n` |
| последняя строка без `\n` | строка без `\n` |
| EOF / пустой файл | `NULL` |
| файл из одних `\n` | `"\n"` на каждый вызов |
| `fd < 0` | `NULL` |
| `BUFFER_SIZE <= 0` | `NULL` |
| `fd >= FD_SETSIZE` (bonus) | `NULL` |
| дескриптор каталога | `NULL` |
| ошибка `read` | `NULL`, внутренний буфер сбрасывается |
| ошибка `malloc` | `NULL`, внутренний буфер сбрасывается |

Возвращённую строку освобождает вызывающий. Если читать файл до `NULL`,
внутреннее состояние освобождается само.

## Что стоит знать

**Файл, прочитанный не до конца, оставляет `stash` в памяти.** Публичного
способа сбросить состояние нет — это следствие подписи функции из сабджекта,
а не утечка в обычном смысле. Дочитал до `NULL` — память свободна.

**Склейка `stash` квадратична.** Каждый `read` создаёт новый `stash` длиной
`len(stash) + BUFFER_SIZE`, поэтому строка длиной *n* при маленьком
`BUFFER_SIZE` стоит O(n²). Замеры при `BUFFER_SIZE=1`:

| длина строки | время |
|---|---|
| 10 000 | 0.20 s |
| 20 000 | 0.69 s |
| 40 000 | 3.04 s |

Рост ×4 при удвоении длины. Тот же файл 1 МБ со строками по 200 000 символов:
`BUFFER_SIZE=32` — 10.7 s, `BUFFER_SIZE=4096` — 0.09 s. На обычной проверке
это не мешает, но тестер с очень длинными строками и `BUFFER_SIZE=1` может
упереться в таймаут.

**После `close(fd)` функция сначала отдаст то, что уже лежит в `stash`,**
и только потом вернёт `NULL`. Буфер уже в памяти, `read` больше не нужен.

## Что проверено

```
norminette *.c *.h
get_next_line.c: OK!              get_next_line_bonus.c: OK!
get_next_line_utils.c: OK!        get_next_line_utils_bonus.c: OK!
get_next_line.h: OK!              get_next_line_bonus.h: OK!
```

Обе части собираются с `-Wall -Wextra -Werror` без единого предупреждения.

Покрыто: `BUFFER_SIZE` = 1, 2, 3, 5, 42, 1024, 4096, 0, −1 · пустой файл ·
файл без `\n` в конце · только `\n` · одна строка · строки по 200 000 символов ·
`fd` = −1 (1000 вызовов), `FD_SETSIZE` · дескриптор каталога · `stdin` через
пайп · чередование трёх fd (bonus).

Во всех прогонах до EOF `malloc` и `free` сбалансированы: `live = 0`.
