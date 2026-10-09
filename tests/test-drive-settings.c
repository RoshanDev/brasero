/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright the Brasero authors */

#include <glib.h>
#include "brasero-drive-settings.h"
#include "brasero-session-helper.h"
#include "brasero-track-data.h"
#include "brasero-track-image.h"

/* Simulate only the hardware identity. The session, tracks, signals and
 * GSettings bindings are real. No drive is opened and no media is written. */
static GObject *fixture_drive;
static GObject *fixture_medium;

BraseroDrive *
brasero_burn_session_get_burner (BraseroBurnSession *session)
{
	return (BraseroDrive *) fixture_drive;
}

BraseroMedium *
brasero_drive_get_medium (BraseroDrive *drive)
{
	return drive ? (BraseroMedium *) fixture_medium : NULL;
}

gboolean
brasero_drive_is_fake (BraseroDrive *drive)
{
	return FALSE;
}

gchar *
brasero_drive_get_display_name (BraseroDrive *drive)
{
	return g_strdup ("Regression fixture");
}

BraseroMedia
brasero_medium_get_status (BraseroMedium *medium)
{
	return BRASERO_MEDIUM_CDR | BRASERO_MEDIUM_BLANK;
}

guint64
brasero_medium_get_max_write_speed (BraseroMedium *medium)
{
	return 1000000;
}

static GSettings *
profile_new (const gchar *type)
{
	GSettings *settings;
	gchar *path;

	path = g_strdup_printf ("/org/gnome/brasero/drives/drive-%u/%s-%i/",
	                        g_str_hash ("Regression fixture"), type,
	                        BRASERO_MEDIUM_TYPE (BRASERO_MEDIUM_CDR));
	settings = g_settings_new_with_path ("org.gnome.brasero.drives", path);
	g_free (path);
	return settings;
}

static void
check_detached_settings (gconstpointer user_data)
{
	BraseroBurnFlag requested = GPOINTER_TO_UINT (user_data);
	BraseroBurnSession *session;
	BraseroDriveSettings *settings;
	BraseroTrackData *data;
	BraseroTrackImage *image;
	GSettings *data_profile;
	GSettings *image_profile;
	GSettings *config;
	gint saved_flags;
	gchar *saved_tmpdir;

	fixture_drive = g_object_new (G_TYPE_OBJECT, NULL);
	fixture_medium = g_object_new (G_TYPE_OBJECT, NULL);
	data_profile = profile_new ("data");
	image_profile = profile_new ("image");
	g_settings_reset (data_profile, "flags");
	g_settings_set_int (image_profile, "flags", BRASERO_BURN_FLAG_NO_TMP_FILES);

	session = brasero_burn_session_new ();
	data = brasero_track_data_new ();
	brasero_track_data_add_fs (data, BRASERO_IMAGE_FS_ISO);
	brasero_burn_session_add_track (session, BRASERO_TRACK (data), NULL);
	settings = brasero_drive_settings_new ();
	brasero_drive_settings_set_session (settings, session);
	brasero_burn_session_set_flags (session, requested);
	saved_flags = g_settings_get_int (data_profile, "flags");
	config = g_settings_new ("org.gnome.brasero.config");
	saved_tmpdir = g_settings_get_string (config, "tmpdir");

	/* The UI releases the profile after options have been confirmed. */
	g_clear_object (&settings);
	brasero_burn_session_push_tracks (session);
	image = brasero_track_image_new ();
	brasero_track_image_set_source (image, "file:///not-written.iso", NULL,
	                               BRASERO_IMAGE_FORMAT_BIN);
	brasero_track_image_set_block_num (image, 202);
	brasero_burn_session_add_track (session, BRASERO_TRACK (image), NULL);
	g_assert_cmpuint (brasero_burn_session_get_flags (session), ==, requested);

	/* Neither direction of the old binding may outlive its owner. */
	brasero_burn_session_set_flags (session, BRASERO_BURN_FLAG_EJECT);
	g_assert_cmpint (g_settings_get_int (data_profile, "flags"), ==, saved_flags);
	g_settings_set_int (data_profile, "flags", 0);
	g_assert_cmpuint (brasero_burn_session_get_flags (session), ==,
	                  BRASERO_BURN_FLAG_EJECT);
	g_object_set (session, "tmpdir", "/tmp/brasero-settings-regression", NULL);
	gchar *current_tmpdir = g_settings_get_string (config, "tmpdir");
	g_assert_cmpstr (current_tmpdir, ==, saved_tmpdir);
	g_free (current_tmpdir);
	g_free (saved_tmpdir);

	g_object_unref (config);
	g_object_unref (data_profile);
	g_object_unref (image_profile);
	g_object_unref (data);
	g_object_unref (image);
	g_object_unref (session);
	g_clear_object (&fixture_drive);
	g_clear_object (&fixture_medium);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);
	g_test_add_data_func ("/settings/detach/keep-open", GUINT_TO_POINTER (0x17),
	                      check_detached_settings);
	g_test_add_data_func ("/settings/detach/merge", GUINT_TO_POINTER (0x1f),
	                      check_detached_settings);
	g_test_add_data_func ("/settings/detach/finalize", GUINT_TO_POINTER (0x07),
	                      check_detached_settings);
	return g_test_run ();
}
