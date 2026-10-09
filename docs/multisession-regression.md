# Multisession regression checks

This series fixes importing previous sessions, the libisofs import crash,
and losing the user's keep-open option while preparing a temporary image.
It also stops a physical recording if an explicit keep-open request loses
its MULTI flag.

## Automated checks

Build with the normal Meson workflow:

```sh
meson setup _build -Dsearch=disabled -Dnautilus=disabled -Dgtk-doc=false \
  -Dyelp=false -Dlibburn=enabled -Dlibisofs=enabled
meson compile -C _build
meson test -C _build --print-errorlogs
```

`drive-settings-detach` runs without a graphical display or optical drive.
It uses the real session, track and GSettings implementation, a memory
settings backend, and simulated hardware identity. It checks keep-open,
merge plus keep-open, and deliberate finalization. Once the settings owner
is released, temporary track replacement must not change the selected
flags; session changes must not update saved preferences and vice versa.
It also checks that the temporary-directory binding is disconnected.

A separate pre-fix reproduction using the production settings code changed
flags from 0x17 to 0x47 when a data track became a generated image and the
image profile did not request MULTI. The fixed path preserves 0x17.

All four Meson checks passed against upstream master 05baa487 plus this
series on Ubuntu 26.04.1, GCC 15.2, GTK 3.24.52, GLib 2.88.0 and
libburn/libisofs 1.5.6. These are local results, not a GNOME CI result.

## Physical regression procedure

Use a disposable blank CD-R and sample files with known checksums:

1. Start the patched executable and verify that its matching libraries are
   loaded. An installed distribution executable is a different test.
2. Create a data project, select keep-open, and burn using libburn.
3. Reinsert the disc. Confirm that the files read back correctly and that
   READ DISC INFORMATION reports an incomplete/appendable disc. Check that
   READ TRACK INFORMATION reports a valid next writable address.
4. Add another file, choose Import, and keep the disc open. Confirm that
   both old and new files remain readable with the expected checksums.
5. Append a file in Windows using the ISO/Joliet multisession workflow.
   Reinsert in Linux and repeat the checks.
6. Import and append once more in Brasero. Check all earlier files,
   including the Windows-added file, and confirm the disc remains open.

The corresponding patches on the 3.12.3 distribution baseline passed this
sequence with a MATSHITA BD-MLT UJ272 drive and a CD-R on 2026-10-09.
After the final append, the next writable address was 36006 and 323838
2048-byte blocks remained free. The recorder logged `Setting multi 1`,
and all five payload files were read and compared with their originals
or the immediately preceding snapshot. File-checksum entries also passed.
The master port was built and tested automatically; the physical sequence
was run on the patched 3.12.3 baseline, not repeated on master.

An earlier run accidentally launched the unpatched distribution executable
and finalized a disc. It is excluded from the successful patched result.

This does not implement Windows Live File System/UDF VAT writes, and does
not establish results for DVD, Blu-ray, audio projects or other backends.
Private disc contents and unredacted application logs are not included.
