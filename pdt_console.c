/*  $Id$

    Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (C): 2011, VU University Amsterdam

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <string.h>
#include <assert.h>

#define ESC 27		/* = '\e', but that is not supported in VC */

typedef struct pdt_console
{ struct pdt_console   *next;
  void *		input_handle;
  void *		output_handle;
  IOFUNCTIONS	        input_functions;
  IOFUNCTIONS	        output_functions;
  IOFUNCTIONS	       *org_input_functions;
  IOFUNCTIONS	       *org_output_functions;
  int			closed;
} pdt_console;

static pdt_console *consoles = NULL;

static pdt_console *
find_console(void *input_handle, void *output_handle)
{ pdt_console *c;

  for(c=consoles; c; c=c->next)
  { if ( c->input_handle == input_handle ||
	 c->output_handle == output_handle )
      return c;
  }

  if ( (c=malloc(sizeof(*c))) )
  { memset(c, 0, sizeof(*c));
    c->input_handle         = input_handle;
    c->output_handle        = output_handle;
    c->input_functions      = *Suser_input->functions;
    c->output_functions     = *Suser_output->functions;
    c->org_input_functions  = Suser_input->functions;
    c->org_output_functions = Suser_output->functions;
    c->next                 = consoles;
    consoles                = c;
  }

  return c;
}


static Sclose_function
free_console(void *input_handle, void *output_handle)
{ pdt_console **pc;
  pdt_console *c;

  for(pc = &consoles; (c=*pc); pc = &c->next)
  { Sclose_function cf = NULL;

    c = *pc;

    if ( input_handle && c->input_handle == input_handle )
    { cf = c->org_input_functions->close;
      c->input_handle = NULL;
      Suser_input->functions = c->org_input_functions;
    } else if ( output_handle && c->output_handle == output_handle )
    { cf = c->org_output_functions->close;
      c->output_handle = NULL;
      Suser_input->functions = c->org_output_functions;
    }

    if ( cf )
    { if ( !c->input_handle && !c->output_handle )
      { *pc = c->next;
	free(c);
      }

      return cf;
    }
  }

  return NULL;
}


static ssize_t
pdt_read(void *handle, char *buf, size_t size)
{ pdt_console *c = find_console(handle, NULL);
  IOSTREAM *out;

  if ( c &&
       PL_ttymode(Suser_input) == PL_RAWTTY  &&
       (out = Suser_output) )
  { ssize_t rc;
    static char go_single[] = {ESC, 's'};

    if ( (*c->org_output_functions->write)(out->handle, go_single, 2) == 2 )
    { rc = (*c->org_input_functions->read)(handle, buf, 2);
      if ( rc == 2 )
	rc = 1;				/* drop \n */
      return rc;
    }
  }

  return (*c->org_input_functions->read)(handle, buf, size);
}


static ssize_t
pdt_write(void *handle, char *buf, size_t size)
{ pdt_console *c = find_console(NULL, handle);
  char *e = buf+size;
  size_t written = 0;

  while(buf < e)
  { char *em;
    ssize_t rc;

    for(em=buf; *em != ESC && em < e; em++)
      ;
    rc = (*c->org_output_functions->write)(handle, buf, em-buf);
    if ( rc < 0 )
      return rc;
    written += (em-buf);
    if ( rc != (em-buf) )
      return written;
    if ( em != e )
    { static char esc[2] = {ESC,ESC};

      if ( (*c->org_output_functions->write)(handle, esc, 2) != 2 )
	return -1;
      em++;
    }
    buf = em;
  }

  return written;
}


static int
pdt_close_in(void *handle)
{ Sclose_function cf = free_console(handle, NULL);

  if ( cf )
    return (*cf)(handle);

  return -1;
}


static int
pdt_close_out(void *handle)
{ Sclose_function cf = free_console(NULL, handle);

  if ( cf )
    return (*cf)(handle);

  return -1;
}


static foreign_t
pdt_wrap_console()
{ IOSTREAM *in, *out;

  if ( (in=Suser_input) && (out=Suser_output) )
  { pdt_console *c;

    assert(in->functions->read != pdt_read);
    assert(out->functions->write != pdt_write);

    if ( (c=find_console(in->handle, out->handle)) )
    { in->functions = &c->input_functions;
      in->functions->read  = pdt_read;
      in->functions->close = pdt_close_in;
      out->functions = &c->output_functions;
      out->functions->write = pdt_write;
      out->functions->close = pdt_close_out;
    }
  }

  return TRUE;
}


install_t
install_pdt_console()
{ PL_register_foreign("pdt_wrap_console", 0, pdt_wrap_console, 0);
}
