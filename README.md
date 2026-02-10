# MineRecord EX

Keeps a record of and tracks in-game character stats during livestreamed gameplay for Minecraft video game. This is a continuation from the old MineRecord Game Tracker project that was started by me back in 2024. This is the new and most up-to-date version of the program. This utility program was developed in Python 3, PostgreSQL and C++. The associated Minecraft Java Edition mod was developed in Java 21. As of right now, this application only has native support for Minecraft Java Edition. If this software gets popular later on in the future, I will go ahead and add support for other titles.

## Disclosure Message:
Please note that a large portion of the codebase for this application was developed with the assistance of Gen AI tools. I just wanted to put this message out there to make users of my software program aware of this. I also wanted to  further clarify that the AI was only used to apply bugfixes for the code and optimize its performance and efficiency. Every other aspect and attribute regarding this code was developed manually by hand by myself. The Gen AI was only used as an assistive tool and it was not used as a replacement.

## Install Instructions:
1. Make sure you have Minecraft Java Edition installed onto your PC (preferrably build version 1.21.10 as its the one that I used to test the functionality of the application). I also highly recommend using prism launcher as Microsoft's official Minecraft launcher is incompatible with this program.
2. Install  64-bit PostgreSQL build version 17.7. You can find the download links [here](https://www.enterprisedb.com/downloads/postgres-postgresql-downloads)
3. Download MineRecordEX and the associated minecraft mod files from the releases page [here](https://github.com/m24faisal/MineRecord-EX/releases/tag/v1.0)
4. (OPTIONAL): If at any point you notice that there is an issue with the Python 3 backend, go ahead and install 64-bit Python 3, version 3.13.7. You can download the installer for it from [here](https://www.python.org/downloads/release/python-3137/). More information for how to set it up can be found below.
5. Install the 64-bit Microsoft Visual C++ Redistributable executable file [here](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version)

## OPTIONAL Python Step
When you run the python installer, make sure when prompted where to install python 3, check the box at the bottom that says to add python to PATH. After you finish installing python 3, reboot your PC and oprn a cmd terminal window. Once you open up the window, type in the following command: 
``` python --version ``` 
This is to check the python version. If you entered it correctly, you should get something like this:
``` Python 3.13.7 ```
and then install the following packages like this:
```
pip install pandas
pip install psycopg2
pip install psycopg2-binary
pip install pybind11
```
After doing so, you should now be able to go ahead and launch the application.

## Usage Instructions
1. Make sure you followed the Installation Instructions noted above before you launch the app for the first
2. Go to File --> "Add Game" and proceed to add your official Minecraft launcher executable file to the main MineRecordEX application table
3. Select the launcher from the MineRecordEX main window's item list, right click and select Start Recording to start recording your game play (if you wish to screen record your gameplay and track your stats) or just run the minecraft instance and enable the mod normally through the Minecraft launch and MineRecordEX should easily detect that the launcher is currently running. The latter option is more meant for users who just want to track player stats and not screen record gameplay. If there are any issues here, please make sure that run as admin is disabled first before running the launcher.
4. After you are done playing a session in Minecraft, go ahead and exit out of the application. Once you have done that, if you are done recording, either right click your minecraft launcher exe in the list and select stop recording, or you can do it through file --> stop recording.
5. After you finished recording or tracking your gameplay, you can go ahead and export the data from settings. You do this by going to file->settings, path settings, and click the export data button.
6. After clicking the button, type in your minecraft username, click ok and it should export the tracked stat data as a csv file and saved it to the specified directory for csv stat files. You can change this directory to whatever you want. The same goes for recordings as well.

For a more visual demonstration, please feel free to watch the demo video on YouTube, linked above. If anyone has any questions about the installation process, please feel free to create an issue on this Github repo and I will respond as soon as I am able to
## Application Demo Video
I uploaded a demo video of how the application works along with all of its functions up on YouTube. You can view it here.

## Application Screenshots

1. Main Window:
<img width="995" height="792" alt="SS 1" src="https://github.com/user-attachments/assets/9bf199a0-bd55-4448-ac39-215304db5dff" />

2. Settings - General
<img width="736" height="631" alt="image" src="https://github.com/user-attachments/assets/404a5622-4c10-4fbd-874d-afe31894abe6" />

3. Settings - Paths
<img width="746" height="645" alt="SS 3" src="https://github.com/user-attachments/assets/e0a1b05b-5228-427e-8030-fed2adc69ce8" />

4. Settings - DB Settings
<img width="752" height="635" alt="SS 5" src="https://github.com/user-attachments/assets/18b45b04-4313-4b26-8d61-f4c44082bc65" />

5. About Page
<img width="606" height="533" alt="SS 5" src="https://github.com/user-attachments/assets/c66f38a4-9c6a-4e46-9e1c-93d08d8b7221" />

## Support The Development
If you enjoyed using my program, please feel free to give me a donation. It really helps to motivate me into putting more features into the application. If interested, you can go ahead and give me donation using the following link:
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W21THQ12)



 

