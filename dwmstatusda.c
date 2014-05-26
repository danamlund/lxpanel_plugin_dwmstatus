/**
 * dwmstatusda
 *
 * Copyright (C) 2014 by Dan Amlund Thomsen <dan@danamlund.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* dwmstatusda my lxde dwmstatus script
 *
 * gcc -Os -Wall -pedantic -std=c99 dwmstatusda.c -lX11 -o dwmstatusda
 */

#define CPUS 4
#define STATUS_MAX_LENGTH 256

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <X11/Xlib.h>

#define MIN(A,B) ((A) < (B) ? (A) : (B))

static Display *dpy;

const char *timeformat = "%a %d %b %H:%M";

unsigned long last_cpu_total[CPUS] = { 0 };
unsigned long last_cpu_idle[CPUS] = { 0 };

void setstatus(char *str) {
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

int snprintf0(char *str, int n, char *fmt, ...) {
  va_list fmtargs;
  char buf[256];
  int len;
  
  va_start(fmtargs, fmt);
  len = vsnprintf(buf, n, fmt, fmtargs);
  va_end(fmtargs);
  
  strncpy(str, buf, len);
  return len;
}

int fill_temp(char *str, int n, const char *sensor) { 
  FILE *sensors = popen("sensors", "r");
  if (NULL == sensors) {
    fprintf(stderr, "Could not open the 'sensors' command\n");
    exit(1);
  }

  char line[256];
  int sensor_len = strlen(sensor);
  while (NULL != fgets(line, 256, sensors)) {
    if (0 == strncmp(sensor, line, sensor_len)) {
      char reading[64];
      if (0 == sscanf(line + sensor_len + 1, "%s", reading)) {
        goto notfound;
      }
      pclose(sensors);
      int readinglen = strlen(reading);
      // remove unicode character and replace it with C or F
      reading[readinglen - 5] = reading[readinglen - 1]; 
      reading[readinglen - 4] = '\0';

      return snprintf0(str, n, "%3s", reading + 1);
    }
  }
 notfound:
  pclose(sensors);
  fprintf(stderr, "Could not find '%s' in sensors\n", sensor);
  exit(1);
}

int fill_mem_usage(char *str, int n) {
  FILE *meminfo = fopen("/proc/meminfo", "r");
  if (meminfo == NULL) {
    fprintf(stderr, "Could not open '/proc/meminfo'\n");
    exit(1);
  }

  int memtotal, memfree, buffers, cached, swaptotal, swapfree;
  char line[256];
  while (NULL != fgets(line, 256, meminfo)) {
    sscanf(line, "MemTotal: %d", &memtotal);
    sscanf(line, "MemFree: %d", &memfree);
    sscanf(line, "Buffers: %d", &buffers);
    sscanf(line, "Cached: %d", &cached);
    sscanf(line, "SwapTotal: %d", &swaptotal);
    sscanf(line, "SwapFree: %d", &swapfree);
  }
  fclose(meminfo);

  float mem_usage = 1.0 - (float) (memfree + buffers + cached) / memtotal;
  float swap_usage = 1.0 - (float) swapfree / swaptotal;

  int single_digit_mem_usage = MIN(9, (int) (mem_usage * 10));
  int single_digit_swap_usage = MIN(9, (int) (swap_usage * 10));

  return snprintf0(str, n, "%d%d", single_digit_mem_usage, 
                   single_digit_swap_usage);
}

int fill_cpu_usage(char *str, int n) { 
  char line[256], cpu_str[10];

  FILE *stat = fopen("/proc/stat", "r");
  if (NULL == stat) {
    fprintf(stderr, "Could not open '/proc/stat'\n");
    exit(1);
  }

  if (NULL == fgets(line, sizeof(line), stat)) {
    fclose(stat);
    fprintf(stderr, "Could not parse '/proc/stat'\n");
    exit(1);
  }
  unsigned long cpu_total[CPUS];
  unsigned long cpu_idle[CPUS];
  for (int cpu = 0; cpu < CPUS; cpu++) {
    if (NULL == fgets(line, sizeof(line), stat)) {
      fclose(stat);
      fprintf(stderr, "Could not find all %d spu cores in '/proc/stat'\n", 
              CPUS);
      exit(1);
    }
    unsigned long user, nice, system, idle;
    sscanf(line, "%s %lu %lu %lu %lu",
           cpu_str, &user, &nice, &system, &idle);
    cpu_total[cpu] = user + nice + system;
    cpu_idle[cpu] = idle;
  }  
  fclose(stat);

  if (0 != last_cpu_total[0] && 0 != last_cpu_idle[0]) {
    for (int cpu = 0; cpu < CPUS; cpu++) {
      float delta_total = cpu_total[cpu] - last_cpu_total[cpu];
      float delta_idle = cpu_idle[cpu] - last_cpu_idle[cpu];
      float cpu_usage = delta_total / (delta_total + delta_idle);
      int single_digit_cpu_usage = (int) (cpu_usage * 10);
      if (single_digit_cpu_usage < 0 || single_digit_cpu_usage > 9) {
        single_digit_cpu_usage = 0;
      }
      str[cpu] = '0' + single_digit_cpu_usage;
    }
  } else {
    for (int cpu = 0; cpu < CPUS; cpu++) {
      str[cpu] = '0';
    }
  }

  for(int cpu = 0; cpu < CPUS; cpu++) {
    last_cpu_total[cpu] = cpu_total[cpu];
    last_cpu_idle[cpu] = cpu_idle[cpu];
  }

  return CPUS;
}

int fill_unread_mail(char *str, int n) { 
  FILE *f = popen("notmuch count tag:inbox", "r");
  if (f == NULL) {
    fprintf(stderr, "Could not open the 'notmuch' command\n");
    exit(1);
  }
  int unread_mail;
  if (0 == fscanf(f, "%d", &unread_mail)) {
    pclose(f);
    fprintf(stderr, "Could not parse notmuch output\n");
    exit(1);
  }
  pclose(f);
  str[0] = unread_mail == 0 ? ' ' : 'M';
  return 1;
}

int fill_date(char *str, int n) {
  struct tm *timtm;
  time_t tim = time(NULL);

  timtm = localtime(&tim);
  if (timtm == NULL) {
    fprintf(stderr, "Could not get localtime()\n");
    exit(1);
  }

  int wrote = strftime(str, n, timeformat, timtm);
  return wrote == 0 ? -1 : wrote;
}

int fill_string(char *str, int n, const char *fill) {
  int i;
  for (i = 0; i < n && fill[i] != '\0'; i++) {
    str[i] = fill[i];
  }
  if (i == n) {
    fprintf(stderr, "Status line too short to fill with '%s'\n", fill);
    exit(1);
  }
  return i;
}

int main(int argc, char **args) {
  dpy = XOpenDisplay(NULL);
  if (NULL == dpy) {
    return 1;
  }

  char buf[STATUS_MAX_LENGTH];

  fill_cpu_usage(buf, sizeof(buf));
  sleep(1);

  int temp_offset, mem_usage_offset, cpu_usage_offset;
  int unread_mail_offset, date_offset;
    
  int filled = 0;

  temp_offset = filled;
  filled += fill_temp(buf + filled, sizeof(buf) - filled, "temp1");

  filled += fill_string(buf + filled, sizeof(buf) - filled, " ");

  mem_usage_offset = filled;
  filled += fill_mem_usage(buf + filled, sizeof(buf) - filled);

  filled += fill_string(buf + filled, sizeof(buf) - filled, " ");

  cpu_usage_offset = filled;
  filled += fill_cpu_usage(buf + filled, sizeof(buf) - filled);

  filled += fill_string(buf + filled, sizeof(buf) - filled, " ");

  unread_mail_offset = filled;
  filled += fill_unread_mail(buf + filled, sizeof(buf) - filled);

  filled += fill_string(buf + filled, sizeof(buf) - filled, " ");

  /* date_offset = filled; */
  /* filled += fill_date(buf + filled, sizeof(buf) - filled); */

  buf[filled] = '\0';
  setstatus(buf);

  unsigned long sleeps = 0;
  while (1) {
    fill_temp(buf + temp_offset, sizeof(buf) - temp_offset, "temp1");
    fill_mem_usage(buf + mem_usage_offset, sizeof(buf) - mem_usage_offset);
    fill_cpu_usage(buf + cpu_usage_offset, sizeof(buf) - cpu_usage_offset);

    if (sleeps % 30 == 0) {
      /* fill_date(buf + date_offset, sizeof(buf) - date_offset); */
      fill_unread_mail(buf + unread_mail_offset, 
                       sizeof(buf) - unread_mail_offset);
    }

    setstatus(buf);
    sleep(2);
    sleeps += 2;
  }
  
  return 0;
}
