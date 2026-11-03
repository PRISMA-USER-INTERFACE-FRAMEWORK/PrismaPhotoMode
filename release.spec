# PrismaPhotoMode release spec. Views nest under PrismaPhotoMode/ inside the views root
# (CreateView "PrismaPhotoMode/index.html"); the INI ships beside the DLL.
target = PrismaPhotoMode
name   = PrismaPhotoMode
dll    = PrismaPhotoMode.dll

assets/views/PrismaPhotoMode => PrismaUI_F4/views/PrismaPhotoMode
config/PrismaPhotoMode.ini   => F4SE/Plugins/PrismaPhotoMode.ini
