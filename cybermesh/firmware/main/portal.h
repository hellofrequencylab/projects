/* Phone-facing side of a box: captive DNS plus a small web app. */
#pragma once

#include "cm.h"

/* Provided by main.c. Hold the lock for every access to the node. */
void cm_box_lock(void);
void cm_box_unlock(void);
cm_node *cm_box_node(void);

void portal_start(void);
void dns_start(void);
