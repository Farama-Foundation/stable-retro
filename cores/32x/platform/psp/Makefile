
# ----------- release -----------
ifneq ($(findstring rel,$(MAKECMDGOALS)),)
ifeq ($(VER),)
$(error need VER)
endif
endif

# ?
rel: ../../EBOOT.PBP readme.txt ../game_def.cfg
	mkdir -p PicoDrive/skin/
	cp $^ PicoDrive/
	cp skin/* PicoDrive/skin/
	zip -9 -r ../../PicoDrive_psp_$(VER).zip PicoDrive
	rm -rf PicoDrive
	mkdir bin_to_cso_mp3
	cp ../../tools/bin_to_cso_mp3/* bin_to_cso_mp3/
	zip -9 -r ../../PicoDrive_psp_$(VER).zip bin_to_cso_mp3
	rm -rf bin_to_cso_mp3

rel_kxploit: readme.txt ../game_def.cfg
	mkdir -p PicoDrive/skin/
	cp $^ PicoDrive/
	cp skin/* PicoDrive/skin/
	zip -9 -r ../../PicoDrive_psp_$(VER)_kxploit.zip PicoDrive
	zip -9 -r ../../PicoDrive_psp_$(VER)_kxploit.zip PicoDrive%
	mkdir bin_to_cso_mp3
	cp ../../tools/bin_to_cso_mp3/* bin_to_cso_mp3/
	zip -9 -r ../../PicoDrive_psp_$(VER)_kxploit.zip bin_to_cso_mp3
	rm -rf bin_to_cso_mp3

