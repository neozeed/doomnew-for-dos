
# D.EXE and DOOM.EXE makefile

# --------------------------------------------------------------------------
#
#      4r  use 80486 timings and register argument passing
#       c  compile only
#      d1  include line number debugging information
#      d2  include full sybolic debugging information
#      ei  force enums to be of type int
#       j  change char default from unsigned to signed
#      oa  relax aliasing checking
#      od  do not optimize
#  oe[=#]  expand functions inline, # = quads (default 20)
#      oi  use the inline library functions
#      om  generate inline 80x87 code for math functions
#      ot  optimize for time
#      ox  maximum optimization
#       s  remove stack overflow checks
#     zp1  align structures on bytes
#      zq  use quiet mode
#  /i=dir  add include directories
#
# --------------------------------------------------------------------------

CCOPTS = /d2 /omaxet /zp1 /4r /ei /j /zq /i=dmx

LOCOBJS = &
 doomdef.obj &
 doomstat.obj &
 dstrings.obj &
 i_ibm.obj &
 i_sound.obj &
 tables.obj &
 f_finale.obj &
 f_wipe.obj  &
 d_main.obj	 &
 d_net.obj	 &
 d_items.obj &
 g_game.obj	 &
 m_menu.obj	 &
 m_misc.obj	 &
 m_argv.obj   &
 m_bbox.obj	 &
 m_fixed.obj &
 m_swap.obj	 &
 m_cheat.obj &
 m_random.obj &
 am_map.obj	 &
 p_ceilng.obj &
 p_doors.obj &
 p_enemy.obj &
 p_floor.obj &
 p_inter.obj &
 p_lights.obj &
 p_map.obj	 &
 p_maputl.obj &
 p_plats.obj &
 p_pspr.obj	 &
 p_setup.obj &
 p_sight.obj &
 p_spec.obj	 &
 p_switch.obj &
 p_mobj.obj	 &
 p_telept.obj &
 p_tick.obj	 &
 p_saveg.obj &
 p_user.obj	 &
 r_bsp.obj	 &
 r_data.obj	 &
 r_draw.obj	 &
 r_main.obj	 &
 r_plane.obj &
 r_segs.obj	 &
 r_sky.obj	 &
 r_things.obj &
 w_wad.obj	 &
 wi_stuff.obj &
 v_video.obj &
 st_lib.obj	 &
 st_stuff.obj &
 hu_stuff.obj &
 hu_lib.obj	 &
 s_sound.obj &
 z_zone.obj	 &
 info.obj		 &
 sounds.obj

d.exe : $(LOCOBJS) i_ibm.obj
 wlink @tic.lnk
 copy d.exe stripd.exe
 wstrip stripd.exe
 4gwbind 4gwpro.exe stripd.exe doom.exe -V 
# sb /R /O doom.exe #Uncomment this to use DOS32/a

i_ibm.obj:
 wcc386 /zp1 /4r /zq /ei /j i_ibm.c

.c.obj :
 wcc386 $(CCOPTS) $[*

.asm.obj :
 tasm /mx $[*

clean : .SYMBOLIC
 del *.obj
 del d.exe
 del stripd.exe
 del doom.exe
 
final : .SYMBOLIC
 wlink @tic.lnk
 copy d.exe stripd.exe
 wstrip stripd.exe
 4gwbind 4gwpro.exe stripd.exe doom.exe -V 
 sb /R /O doom.exe
