/*
 * TilEm II
 *
 * Copyright (c) 2011 Benjamin Moody
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

G_BEGIN_DECLS

#define TILEM_TYPE_ANIMATION           (tilem_animation_get_type())
#define TILEM_ANIMATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), TILEM_TYPE_ANIMATION, TilemAnimation))
#define TILEM_ANIMATION_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST((cls), TILEM_TYPE_ANIMATION, TilemAnimationClass))
#define TILEM_IS_ANIMATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), TILEM_TYPE_ANIMATION))
#define TILEM_IS_ANIMATION_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE((cls), TILEM_TYPE_ANIMATION))
#define TILEM_ANIMATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), TILEM_TYPE_ANIMATION, TilemAnimationClass))

typedef struct _TilemAnimation TilemAnimation;
typedef struct _TilemAnimationClass TilemAnimationClass;
typedef struct _TilemAnimFrame TilemAnimFrame;


/* Create a new TilemAnimation for the given display dimensions. */
TilemAnimation * tilem_animation_new(int display_width,
                                     int display_height);

/* Add a frame to the animation.  BUF holds the LCD contents, DURATION
   is the length of time this frame should be displayed (in
   milliseconds.) */
gboolean tilem_animation_append_frame(TilemAnimation *anim,
                                      const TilemLCDBuffer *buf,
                                      int duration);

/* Set output image size. */
void tilem_animation_set_size(TilemAnimation *anim, int width, int height);

/* Set output image colors. */


/* Set animation speed factor */
void tilem_animation_set_speed(TilemAnimation *anim, gdouble factor);

/* Get animation speed factor */
gdouble tilem_animation_get_speed(TilemAnimation *anim);

/* Retrieve the next frame of the animation.  If FRM is NULL, retrieve
   the first frame.  If FRM is non-null, it must be a frame belonging
   to this animation. */
TilemAnimFrame *tilem_animation_next_frame(TilemAnimation *anim,
                                           TilemAnimFrame *frm);

/* Get the duration of this frame (milliseconds by the original
   clock.) */
int tilem_anim_frame_get_duration(TilemAnimFrame *frm);

/* Convert frame to an indexed-color image buffer.  FRM must be a
   frame belonging to this animation.  The returned buffer must be
   freed with g_free(). */
void tilem_animation_get_indexed_image(TilemAnimation *anim,
                                       TilemAnimFrame *frm,
                                       byte **buffer,
                                       int *width, int *height);

/* Save animation to a file.  TYPE is an ASCII string describing the
   type.  Options are specified by OPTION_KEYS and OPTION_VALUES (see
   gdk_pixbuf_savev().) */
gboolean tilem_animation_save(TilemAnimation *anim,
                              const char *fname, const char *type,
                              char **option_keys, char **option_values,
                              GError **err);

G_END_DECLS
