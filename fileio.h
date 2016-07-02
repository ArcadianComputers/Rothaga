/*
 * fileio.h
 *
 *  Created on: Apr 16, 2016
 *      Author: Admin
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ncurses.h>

#ifndef NETIO_H_
#define NETIO_H_
#include "netio.h"
#endif

#include "Images/a.txt.h"
#include "Images/b.txt.h"
#include "Images/c.txt.h"
#include "Images/d.txt.h"
#include "Images/e.txt.h"
#include "Images/f.txt.h"
#include "Images/g.txt.h"
#include "Images/h.txt.h"
#include "Images/i.txt.h"
#include "Images/j.txt.h"
#include "Images/k.txt.h"
#include "Images/l.txt.h"
#include "Images/m.txt.h"
#include "Images/n.txt.h"
#include "Images/o.txt.h"
#include "Images/p.txt.h"
#include "Images/q.txt.h"
#include "Images/r.txt.h"
#include "Images/s.txt.h"
#include "Images/t.txt.h"
#include "Images/u.txt.h"
#include "Images/v.txt.h"
#include "Images/w.txt.h"
#include "Images/x.txt.h"
#include "Images/y.txt.h"
#include "Images/z.txt.h"

void load_config(char *);
void kill_server(int);
void build_ui(RothagaClient *);
void draw_ui(RothagaClient *);
void backspace(RothagaClient *);
void rprintw(WINDOW *, RothagaClient *, char *);
void kill_rothaga(int);
void clear_chat(RothagaClient *);

#endif /* FILEIO_H_ */
