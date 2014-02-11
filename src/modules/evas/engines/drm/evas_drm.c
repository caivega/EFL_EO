#include "evas_engine.h"
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/mman.h>

/* NB: REALLY hate to store this here, but sigaction signal handlers cannot 
 * pass any 'user data' to the signal handlers :( */
static Evas_Engine_Info_Drm *siginfo;

static int 
_evas_drm_card_open(int card)
{
   char dev[32];
   int fd = -1;
   uint64_t dumb;

   sprintf(dev, DRM_DEV_NAME, DRM_DIR_NAME, card);

   if ((fd = open(dev, (O_RDWR | O_CLOEXEC))) < 0)
     {
        CRI("Could not open drm device %s: %m", dev);
        return -1;
     }

   drmVersionPtr ver;
   if ((ver = drmGetVersion(fd)))
     {
        DBG("Drm Driver Name: %s", ver->name);
        drmFreeVersion(ver);
     }

   /* check for dumb buffer support
    * 
    * NB: This checks that we can at least support software rendering */
   if ((drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &dumb) < 0) || (!dumb))
     {
        CRI("Drm Device %s does not support software rendering", dev);

        /* close the card */
        close(fd);

        /* return failure */
        return -1;
     }

   DBG("Opened Drm Card %s: %d", dev, fd);

   /* return opened card */
   return fd;
}

static int 
_evas_drm_tty_open(Evas_Engine_Info_Drm *info)
{
   int tty = STDIN_FILENO;

   /* check if the current stdin is a valid tty */
   if (!isatty(tty))
     {
        /* if not, try to open the curren tty */
        if ((tty = open("/dev/tty", (O_RDWR | O_CLOEXEC))) < 0)
          {
             int tty0 = -1, num = -1;
             char name[16];

             /* if that fails, try tty0 */
             if ((tty0 = open("/dev/tty0", (O_WRONLY | O_CLOEXEC))) < 0)
               {
                  CRI("Could not open tty0: %m");
                  return -1;
               }

             /* try to find a non-opened tty */
             if ((ioctl(tty0, VT_OPENQRY, &num) < 0) || (num < 0))
               {
                  CRI("Could not find a non-opened tty");
                  close(tty0);
                  return -1;
               }

             snprintf(name, sizeof(name), "/dev/tty%d", num);

             /* try to open this tty */
             if ((tty = open(name, (O_RDWR | O_CLOEXEC))) < 0)
               {
                  CRI("Could not open tty: %s", name);
                  close(tty0);
                  return -1;
               }

             /* set flag that evas should close this tty */
             info->info.own_tty = EINA_TRUE;

             /* close /dev/tty0 */
             close(tty0);
          }
     }
   else
     {
        /* set flag that evas should close this tty */
        info->info.own_tty = EINA_TRUE;
     }

   DBG("Opened Tty %d", tty);

   return tty;
}

static int 
_evas_drm_crtc_find(int fd, drmModeRes *res, drmModeConnector *conn)
{
   int crtc = -1;
   drmModeEncoder *enc = NULL;

   /* if this connector already has an encoder, get it */
   if (conn->encoder_id) enc = drmModeGetEncoder(fd, conn->encoder_id);

   /* if this encoder already has a crtc, lets try to use that */
   if ((enc) && (enc->crtc_id)) crtc = enc->crtc_id;

   if (crtc < 0)
     {
        int i = 0, c = 0;

        /* if this connector has no encoder, we need to find one */
        for (; i < conn->count_encoders; ++i)
          {
             /* try to get this encoder */
             if (!(enc = drmModeGetEncoder(fd, conn->encoders[i])))
               continue;

             /* loop global crtcs */
             for (; c < res->count_crtcs; ++c)
               {
                  /* does this crtc work with this encoder ? */
                  if (!(enc->possible_crtcs & (1 << c))) continue;

                  /* FIXME: We could be more proactive here and check that 
                   * nobody else is using this crtc */

                  /* if it works, let's use it */
                  crtc = res->crtcs[c];
                  break;
               }

             if (crtc >= 0) break;
          }
     }

   /* free the encoder */
   if (enc) drmModeFreeEncoder(enc);

   return crtc;
}

static void 
_evas_drm_tty_sigusr1(int x EINA_UNUSED, siginfo_t *info, void *data EINA_UNUSED)
{
   Evas_Engine_Info_Drm *einfo;

   DBG("Caught SIGUSR1");

   if (!(einfo = siginfo)) return;

   /* TODO: set canvas to not render */

   DBG("\tDrop Master & Release VT");

   /* drop drm master */
   if (einfo->info.own_fd)
     {
        if (drmDropMaster(einfo->info.fd) != 0)
          WRN("Could not drop drm master: %m");
     }

   /* release vt */
   if (einfo->info.own_tty)
     {
        if (ioctl(einfo->info.tty, VT_RELDISP, 1) < 0)
          WRN("Could not release vt: %m");
     }
}

static void 
_evas_drm_tty_sigusr2(int x EINA_UNUSED, siginfo_t *info, void *data EINA_UNUSED)
{
   Evas_Engine_Info_Drm *einfo;

   DBG("Caught SIGUSR2");

   if (!(einfo = siginfo)) return;

   /* TODO: set canvas to render again */

   DBG("\tAcquire VT & Set Master");

   /* acquire vt */
   if (einfo->info.own_tty)
     {
        if (ioctl(einfo->info.tty, VT_RELDISP, VT_ACKACQ) < 0)
          WRN("Could not acquire vt: %m");
     }

   /* set master */
   if (einfo->info.own_fd)
     {
        if (drmSetMaster(einfo->info.fd) != 0)
          WRN("Could not set drm master: %m");
     }
}

static Eina_Bool 
_evas_drm_tty_setup(Evas_Engine_Info_Drm *info)
{
   struct vt_mode vtmode = { 0 };
   struct sigaction sig;

   /* check for valid tty */
   if (info->info.tty < 0) return EINA_FALSE;

#if 0
   /* set vt to graphics mode */
   if (ioctl(info->info.tty, KDSETMODE, KD_GRAPHICS) < 0)
     {
        CRI("Could not set tty to graphics mode: %m");
        return EINA_FALSE;
     }
#endif

   /* setup tty rel/acq signals */
   vtmode.mode = VT_PROCESS;
   vtmode.waitv = 0;
   vtmode.relsig = SIGUSR1;
   vtmode.acqsig = SIGUSR2;
   if (ioctl(info->info.tty, VT_SETMODE, &vtmode) < 0)
     {
        CRI("Could not set tty mode: %m");
        return EINA_FALSE;
     }

   /* store info struct 
    * 
    * NB: REALLY hate to store this here, but sigaction signal handlers cannot 
    * pass any 'user data' to the signal handlers :(
    */
   siginfo = info;

   /* setup signal handlers for above signals */
   sig.sa_sigaction = _evas_drm_tty_sigusr1;
   sig.sa_flags = (SA_NODEFER | SA_SIGINFO | SA_RESTART);
   sigemptyset(&sig.sa_mask);
   sigaction(SIGUSR1, &sig, NULL);

   sig.sa_sigaction = _evas_drm_tty_sigusr2;
   sig.sa_flags = (SA_NODEFER | SA_SIGINFO | SA_RESTART);
   sigemptyset(&sig.sa_mask);
   sigaction(SIGUSR2, &sig, NULL);

   return EINA_TRUE;
}

static void 
_evas_drm_outbuf_page_flip(int fd EINA_UNUSED, unsigned int seq EINA_UNUSED, unsigned int tv_sec EINA_UNUSED, unsigned int tv_usec EINA_UNUSED, void *data)
{
   Outbuf *ob;

   /* get the output buffer from data */
   if (!(ob = data)) return;

   /* DBG("Page Flip Event"); */

   ob->priv.pending_flip = EINA_FALSE;
   ob->priv.curr = (ob->priv.curr + 1) % ob->priv.num;
}

static void 
_evas_drm_outbuf_vblank(int fd EINA_UNUSED, unsigned int frame EINA_UNUSED, unsigned int sec EINA_UNUSED, unsigned int usec EINA_UNUSED, void *data)
{
   Outbuf *ob;

   /* get the output buffer from data */
   if (!(ob = data)) return;

   DBG("VBlank Event");
}

Eina_Bool 
evas_drm_init(Evas_Engine_Info_Drm *info, int card)
{
   /* check for valid engine info */
   if (!info) return EINA_FALSE;

   setvbuf(stdout, NULL, _IONBF, 0);

   /* check if we already opened the card */
   if (info->info.fd < 0)
     {
        /* try to open the drm card */
        if ((info->info.fd = _evas_drm_card_open(card)) < 0) 
          return EINA_FALSE;

        /* set flag to indicate that evas opened the card and we should 
         * be the one to close it */
        info->info.own_fd = EINA_TRUE;
     }

   /* check if we already opened the tty */
   if (info->info.tty < 0)
     {
        /* try to open the current tty */
        if ((info->info.tty = _evas_drm_tty_open(info)) < 0) 
          {
             /* check if we already opened the card. if so, close it */
             if ((info->info.fd >= 0) && (info->info.own_fd))
               {
                  close(info->info.fd);
                  info->info.fd = -1;
               }

             return EINA_FALSE;
          }
     }

   /* with the tty opened, we need to set it up */
   if (!_evas_drm_tty_setup(info))
     {
        /* setup of tty failed, close it */
        if ((info->info.tty >= 0) && (info->info.own_tty))
          close(info->info.tty);

        /* FIXME: Close card also ?? */

        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool 
evas_drm_shutdown(Evas_Engine_Info_Drm *info)
{
   /* check for valid engine info */
   if (!info) return EINA_TRUE;

   /* check if we already opened the tty. if so, close it */
   if ((info->info.tty >= 0) && (info->info.own_tty))
     {
        close(info->info.tty);
        info->info.tty = -1;
     }

   /* check if we already opened the card. if so, close it */
   if ((info->info.fd >= 0) && (info->info.own_fd))
     {
        close(info->info.fd);
        info->info.fd = -1;
     }

   return EINA_TRUE;
}

Eina_Bool 
evas_drm_outbuf_setup(Outbuf *ob)
{
   drmModeRes *res;
   drmModeConnector *conn;
   int i = 0;

   /* check for valid Output buffer */
   if ((!ob) || (ob->priv.fd < 0)) return EINA_FALSE;

   /* setup drmHandleEvent context */
   memset(&ob->priv.ctx, 0, sizeof(ob->priv.ctx));
   ob->priv.ctx.version = DRM_EVENT_CONTEXT_VERSION;
   ob->priv.ctx.page_flip_handler = _evas_drm_outbuf_page_flip;
   ob->priv.ctx.vblank_handler = _evas_drm_outbuf_vblank;

   /* try to get drm resources */
   if (!(res = drmModeGetResources(ob->priv.fd)))
     {
        CRI("Could not get drm resources: %m");
        return EINA_FALSE;
     }

   /* loop the connectors */
   for (; i < res->count_connectors; ++i)
     {
        int crtc = -1;
        int m = 0;

        /* try to get this connector */
        if (!(conn = drmModeGetConnector(ob->priv.fd, res->connectors[i])))
          {
             WRN("Could not get drm connector %d: %m", i);
             continue;
          }

        /* make sure this connector is actually connected */
        if (conn->connection != DRM_MODE_CONNECTED) 
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* make sure it has modes */
        if (conn->count_modes == 0)
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* try to find a crtc for this connector */
        if ((crtc = _evas_drm_crtc_find(ob->priv.fd, res, conn)) < 0) 
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* record the connector id */
        ob->priv.conn = conn->connector_id;

        /* record the crtc id */
        ob->priv.crtc = crtc;

        /* spew out connector properties for testing */
        /* drmModePropertyPtr props; */
        /* for (m = 0; m < conn->count_props; m++) */
        /*   { */
        /*      props = drmModeGetProperty(ob->priv.fd, conn->props[m]); */
        /*      if (!props) continue; */
        /*      DBG("Property Name: %s", props->name); */
        /*   } */

        /* record the current mode */
        memcpy(&ob->priv.mode, &conn->modes[0], sizeof(ob->priv.mode));
        DBG("Output Current Mode: %d: %d %d", ob->priv.conn, 
            conn->modes[0].hdisplay, conn->modes[0].vdisplay);

        for (m = 0; m < conn->count_modes; m++)
          {
             DBG("Output Available Mode: %d: %d %d %d", ob->priv.conn, 
                 conn->modes[m].hdisplay, conn->modes[m].vdisplay, 
                 conn->modes[m].vrefresh);

             /* try to find a mode which matches the requested size */
             if ((conn->modes[m].hdisplay == ob->w) && 
                 (conn->modes[m].vdisplay == ob->h) && 
                 (conn->modes[m].vrefresh == 60))
               {
                  memcpy(&ob->priv.mode, &conn->modes[m], 
                         sizeof(ob->priv.mode));
               }
          }

        /* free connector resources */
        drmModeFreeConnector(conn);

        break;
     }

   /* free drm resources */
   drmModeFreeResources(res);

   return EINA_TRUE;
}

void 
evas_drm_outbuf_framebuffer_set(Outbuf *ob, Buffer *buffer)
{
   int ret;

   /* validate params */
   if ((!ob) || (!buffer)) return;

   ret = drmModeSetCrtc(ob->priv.fd, ob->priv.crtc, buffer->fb, 0, 0, 
                        &ob->priv.conn, 1, &ob->priv.mode);

   if (ret) ERR("Failed to set crtc: %m");
   else buffer->valid = EINA_TRUE;
}

Eina_Bool 
evas_drm_framebuffer_create(int fd, Buffer *buffer, int depth)
{
   struct drm_mode_create_dumb carg;
   struct drm_mode_destroy_dumb darg;
   struct drm_mode_map_dumb marg;

   /* check for valid info */
   if (fd < 0) return EINA_FALSE;

   /* try to create a dumb buffer */
   memset(&carg, 0, sizeof(carg));
   carg.width = buffer->w;
   carg.height = buffer->h;
   carg.bpp = depth;
   if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &carg) < 0)
     {
        ERR("Could not create dumb buffer: %m");
        return EINA_FALSE;
     }

   buffer->stride = carg.pitch;
   buffer->size = carg.size;
   buffer->handle = carg.handle;

   /* try to create a framebuffer object */
   /* FIXME: Hardcoded bpp */
   if (drmModeAddFB(fd, buffer->w, buffer->h, 24, depth, buffer->stride, 
                    buffer->handle, &buffer->fb))
     {
        ERR("Could not create framebuffer object: %m");
        goto add_err;
     }

   DBG("Creating dumb buffer: %d %d %d %d", buffer->fb, 
       buffer->w, buffer->h, depth);

   /* try to mmap the buffer */
   memset(&marg, 0, sizeof(marg));
   marg.handle = buffer->handle;
   if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &marg))
     {
        ERR("Could not map dumb buffer: %m");
        goto map_err;
     }

   /* do actual mmap of memory */
   buffer->data = 
     mmap(0, buffer->size, (PROT_WRITE), 
//     mmap(0, buffer->size, (PROT_READ | PROT_WRITE), 
          MAP_SHARED, fd, marg.offset);
   if (buffer->data == MAP_FAILED)
     {
        ERR("Could not mmap dumb buffer: %m");
        goto map_err;
     }

   /* clear memory */
   memset(buffer->data, 0, buffer->size);

   return EINA_TRUE;

map_err:
   /* remove the framebuffer */
   drmModeRmFB(fd, buffer->fb);

add_err:
   /* destroy buffer */
   memset(&darg, 0, sizeof(darg));
   darg.handle = buffer->handle;
   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);

   return EINA_FALSE;
}

void 
evas_drm_framebuffer_destroy(int fd, Buffer *buffer)
{
   struct drm_mode_destroy_dumb darg;

   /* check for valid info */
   if (fd < 0) return;

   /* unmap the buffer data */
   if (buffer->data) munmap(buffer->data, buffer->size);

   /* remove the framebuffer */
   drmModeRmFB(fd, buffer->fb);

   /* destroy buffer */
   memset(&darg, 0, sizeof(darg));
   darg.handle = buffer->handle;
   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
}

Eina_Bool 
evas_drm_framebuffer_send(Outbuf *ob, Buffer *buffer, Eina_Rectangle *rects, unsigned int count)
{
   int ret;

   /* check for valid Output buffer */
   if ((!ob) || (ob->priv.fd < 0)) return EINA_FALSE;

   /* check for valid buffer */
   if ((!buffer) || (!buffer->valid)) return EINA_FALSE;

#ifdef DRM_MODE_FEATURE_DIRTYFB
   drmModeClip *clip;
   unsigned int i = 0;

   /* WRN("drmModeDirtyFB is experimental"); */

   /* NB: alloca automatically frees memory */
   clip = alloca(count * sizeof(drmModeClip));
   for (i = 0; i < count; i++)
     {
        clip[i].x1 = rects[i].x;
        clip[i].y1 = rects[i].y;
        clip[i].x2 = rects[i].w;
        clip[i].y2 = rects[i].h;
     }

   ret = drmModeDirtyFB(ob->priv.fd, buffer->fb, clip, count);
   if (ret)
     {
        if (ret == -EINVAL)
          ERR("Could not set FB Dirty: %m");
     }
#endif

   ret = drmModePageFlip(ob->priv.fd, ob->priv.crtc, buffer->fb, 
                         DRM_MODE_PAGE_FLIP_EVENT, ob);
   if (ret)
     {
        ERR("Cannot flip crtc for connector %u: %m", ob->priv.conn);
        return EINA_FALSE;
     }

   ob->priv.sent = buffer;
   ob->priv.pending_flip = EINA_TRUE;

//   while (ob->priv.pending_flip)
     drmHandleEvent(ob->priv.fd, &ob->priv.ctx);

   return EINA_TRUE;
}