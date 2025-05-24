# XXAnimRootEditor
A plugin for Unreal Engine used to remove root motion，while keep the final pose, by accumulating root transform on the child bones.

# Prerequisites
Unreal Engine 5.2+

# Usage
Get the plugin by clone repository or Download Zip, extract to the `Plugins` folder in your project directory.
## Editor Tab
- select an animation sequence in content browser, then click the icon in LevelEditor or Window menu.
  
  ![image](https://github.com/user-attachments/assets/15d969bb-80e6-4ff2-80ba-90a939dbdd8c)
  ![image](https://github.com/user-attachments/assets/f2a94933-ee2a-422b-868a-4573c2a37d9e)

- Slide the progress bar and select the range of frame.

  ![image](https://github.com/user-attachments/assets/fbfdcf99-dbf6-4dd8-be4c-e0905f052b6e)

- Check translation or rotation to remove, then click"Remove Root Motion".

  ![image](https://github.com/user-attachments/assets/820e1e2f-071b-4667-b088-af52fbc424de)


## Animation Modifer
Animation modifer is also available, by adding "XXRemoveRootMotionModifier".

![image](https://github.com/user-attachments/assets/93f237be-b289-4282-9d0c-ee405e331506)

# Preview
Source:

![Attack_Release_Game_2](https://github.com/user-attachments/assets/bbf56f86-1cc2-4a73-b07c-f4c9c2f8bf86)

Modified:

![Attack_Release_Game_3](https://github.com/user-attachments/assets/3d5b092d-3fae-48e0-bc37-b72188dcab0c)

# More Infomation
Reference to [https://zhuanlan.zhihu.com/p/1907297147545711776](https://zhuanlan.zhihu.com/p/1907297147545711776)
