# ����
 
 С��èC++��ҪQt 5(>=5.12)

# Windows

 ��ʹ��msys2��������°��GCC��MinGW-w64������������С��èC++��VC�������汾��gcc��һ���ܹ��������롣

 ���벽�裺
 - ��װmsys2 (https://www.msys2.org)
 - ʹ��msys2��pacman����װmingw-w64-x86_64-qt5��mingw-w64-x86_64-gcc
 - ��װqtcreator
 - ʹ��qtcreator��Red_Panda_CPP.pro�ļ�

# Linux

����:
 - ��װ gcc �� qt5������ذ�
 - ʹ��qtcreator��Red_Panda_CPP.pro�ļ�


## Ubuntu

### 1.��װ������

```text
apt install gcc g++ make gdb gdbserver 
```

### 2.��װQT5��������

```text
apt install qtbase5-dev qttools5-dev-tools  libicu-dev libqt5svg5-dev  git qterminal
```

### 3.����Դ��

```
git clone https://gitee.com/royqh1979/RedPanda-CPP.git
```

### 4.����

```
cd cd RedPanda-CPP/
qmake Red_Panda_CPP.pro 
make -j8
sudo make install
```

### 5.����

```
RedPandaIDE
```