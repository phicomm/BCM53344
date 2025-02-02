/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "lib.h"
#include "version.h"
#include "log.h"

#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#include "mstpd/mstpd.h"

/* Global variable container. */
extern struct lib_globals *mstpm;
extern s_int32_t mstp_vty_port;

int mstp_start (s_int32_t, char *, s_int32_t, char *, u_int16_t);
void mstp_stop (void);

#ifdef HAVE_GETOPT_H
/* stpd options. */
static struct option longopts[] =
{

  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "version",     no_argument,       NULL, 'v'},
  { "ha_cold",     no_argument,       NULL, 'c'}, /* HA cold start if used. */
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* Help information display. */
void
mstp_usage (int status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages MSTP \n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-P, --vty_port     Set vty's port number\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
-c, --ha_cold      High availability - cold start\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }

  exit (status);
}

/* SIGHUP handler. */
void
sighup (int sig)
{
  zlog_info (mstpm, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog_info (mstpm, "Terminating on signal");

  /* Stop MSTP module.  */
  mstp_stop ();

  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_rotate (mstpm, mstpm->log);
}

/* Initialization of signal handles.  */
void
signal_init ()
{
  pal_signal_init ();
  pal_signal_set (SIGHUP, sighup);
  pal_signal_set (SIGINT, sigint);
  pal_signal_set (SIGTERM, sigint);
  pal_signal_set (SIGUSR1, sigusr1);
}

/* Main routine of mstpd. */
int
main (int argc, char **argv)
{
  result_t ret;
  char *p;
  s_int32_t daemon_mode = 0;
  char *config_file = NULL;
  u_int16_t ha_cold_l = 0;
  s_int32_t vty_port = MSTP_VTY_PORT;
  char *progname;

  /* Set umask before anything for security */
  umask (0027);

#ifdef VTYSH
  /* Unlink vtysh domain socket. */
  unlink (MSTP_VTYSH_PATH);
#endif /* VTYSH */

  /* Get program name. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Command line option parse. */
  while (1)
    {
      int opt;

#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "df:hP:rvc", longopts, 0);
#else
      opt = getopt (argc, argv, "df:hP:rvc");
#endif /* HAVE_GETOPT_H */

      if (opt == EOF)
        break;

      switch (opt)
        {
        case 0:
          break;
         case 'c':
          ha_cold_l = 1;
          break;
        case 'd':
          daemon_mode = 1;
          break;
        case 'f':
          config_file = optarg;
          break;
        case 'P':
          vty_port = atoi (optarg);
          break;
        case 'v':
          print_version (progname);
          exit (0);
        case 'h':
          mstp_usage (0, progname);
          break;
        default:
          mstp_usage (1, progname);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  /* Start MSTP module.  */
  ret = mstp_start (daemon_mode, config_file, vty_port, progname, ha_cold_l);

  /* Not reached. */
  exit (0);
}
