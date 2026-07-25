VMAT GitHub setup for Windows
================================

GitHub repository:
https://github.com/Memphis-Cat/.vmat_converter-to-BSP

Local project directory:
D:\VMAT

Requirements
------------
Install Git for Windows:
https://git-scm.com/download/win

One-time setup
--------------
1. Copy these BAT files and .gitignore into D:\VMAT.
2. Run 01_setup_project.bat.
3. Run 04_make_src.bat if you want to create the src folder separately.
4. Put your own C++ code under D:\VMAT\src.
5. Run 05_add_source_sdk_2013.bat.
6. Run 02_upload_to_github.bat.

Everyday use
------------
Upload your changes:
    Run 02_upload_to_github.bat

Download GitHub changes:
    Run 03_download_from_github.bat

Important
---------
The Source SDK folder is a submodule. Your repository stores a link to the
Valve repository and a selected commit. It does not upload Valve's complete
SDK into your own Git history.

A new computer should clone using:
git clone --recurse-submodules https://github.com/Memphis-Cat/.vmat_converter-to-BSP.git D:\VMAT

Git identity, if Git asks for it:
git config --global user.name "Your Name"
git config --global user.email "your-email@example.com"

GitHub authentication
---------------------
On the first push, Git for Windows may open a browser so you can sign in to
GitHub. Do not put a password or access token inside these BAT files.
