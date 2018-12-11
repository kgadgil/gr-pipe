/* -*- c++ -*- */
/*
 * Copyright 2004,2010 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <iostream>
#include <pipe_sink.h>
#include <gnuradio/io_signature.h>

/*namespace gr{
  namespace pipe {*/
/*
 * Create a new instance of pipe_sink and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
pipe_sink_sptr 
pipe_make_sink (size_t in_item_sz,
                const char *cmd)
{
  return gnuradio::get_initial_sptr(new pipe_sink (in_item_sz, cmd));
}

/*
 * Specify constraints on number of input and output streams.
 * This info is used to construct the input and output signatures
 * (2nd & 3rd args to gr::block's constructor).  The input and
 * output signatures are used by the runtime system to
 * check that a valid number and type of inputs and outputs
 * are connected to this block.
 */
static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams
static const int MIN_OUT = 0;	// minimum number of output streams
static const int MAX_OUT = 0;	// maximum number of output streams

/*
 * The private constructor
 */

pipe_sink::pipe_sink (size_t in_item_sz,
                      const char *cmd)
  : gr::sync_block ("pipe_sink",
                   gr::io_signature::make (MIN_IN,  MAX_IN,  in_item_sz),
                   gr::io_signature::make (MIN_OUT, MAX_OUT, 0)),
    d_in_item_sz (in_item_sz)
{
  create_command_process(cmd);
}

/*
 * Our virtual destructor.
 */
pipe_sink::~pipe_sink ()
{
  long ret;
  int pstat;

  // Set file descriptors to blocking, to be sure to consume
  // the remaining output generated by the process.
  reset_fd_flags(d_cmd_stdin_pipe[1], O_NONBLOCK);
  fclose(d_cmd_stdin);

  do {
    ret = waitpid(d_cmd_pid, &pstat, 0);
  } while (ret == -1 && errno == EINTR);

  if (ret == -1) {
    perror("waitpid()");
    return ;
  }

  if (WIFEXITED(pstat))
    std::cerr << "Process exited with code " << WEXITSTATUS(pstat) << std::endl;
  else
    std::cerr << "Abnormal process termination" << std::endl;
}

void
pipe_sink::set_unbuffered(bool unbuffered)
{
  d_unbuffered = unbuffered;
}

void
pipe_sink::set_fd_flags(int fd, long flags)
{
  long ret;

  ret = fcntl(fd, F_GETFL);
  if (ret == -1) {
    perror("fcntl()");
    throw std::runtime_error("fcntl() error");
  }

  ret = fcntl(fd, F_SETFL, ret | flags);
  if (ret == -1) {
    perror("fcntl()");
    throw std::runtime_error("fcntl() error");
  }
}

void
pipe_sink::reset_fd_flags(int fd, long flag)
{
  long ret;

  ret = fcntl(fd, F_GETFL);
  if (ret == -1) {
    perror("fcntl()");
    throw std::runtime_error("fcntl() error");
  }

  ret = fcntl(fd, F_SETFL, ret & ~flag);
  if (ret == -1) {
    perror("fcntl()");
    throw std::runtime_error("fcntl() error");
  }
}

void
pipe_sink::create_pipe(int pipefd[2])
{
  int ret;

  ret = pipe(pipefd);
  if (ret != 0) {
    perror("pipe()");
    throw std::runtime_error("pipe() error");
  }
}

void
pipe_sink::create_command_process(const char *cmd)
{
  create_pipe(d_cmd_stdin_pipe);

  d_cmd_pid = fork();
  if (d_cmd_pid == -1) {
    perror("fork()");
    return ;
  }
  else if (d_cmd_pid == 0) {
    dup2(d_cmd_stdin_pipe[0], STDIN_FILENO);
    close(d_cmd_stdin_pipe[0]);
    close(d_cmd_stdin_pipe[1]);

    execl("/bin/sh", "sh", "-c", cmd, NULL);

    perror("execl()");
    exit(EXIT_FAILURE);
  }
  else {
    close(d_cmd_stdin_pipe[0]);
    set_fd_flags(d_cmd_stdin_pipe[1], O_NONBLOCK);

    fcntl(d_cmd_stdin_pipe[1], F_SETFD, FD_CLOEXEC);

    d_cmd_stdin = fdopen(d_cmd_stdin_pipe[1], "w");
    if (d_cmd_stdin == NULL) {
      perror("fdopen()");
      throw std::runtime_error("fdopen() error");
      return ;
    }
  }
}


int
pipe_sink::write_process_input(const uint8_t *in, int nitems)
{
  size_t ret;

  ret = fwrite(in, d_in_item_sz, nitems, d_cmd_stdin);
  if (    ret == 0
       && ferror(d_cmd_stdin)
       && errno != EAGAIN
       && errno != EWOULDBLOCK) {
    throw std::runtime_error("fwrite() error");
    return (-1);
  }

  if (d_unbuffered)
    fflush(d_cmd_stdin);

  return (ret);
}


int 
pipe_sink::work (int noutput_items,
                 gr_vector_const_void_star &input_items,
                 gr_vector_void_star &output_items)
{
  const uint8_t *in = (const uint8_t *) input_items[0];
  int n_consumed;

  n_consumed = write_process_input(in, noutput_items);

  return (n_consumed);
}
/*}
}*/
