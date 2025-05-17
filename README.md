# Установка программного обеспечения для мобильной платформы KUKA youBot

В данном документе представлена инструкция по установке и настройке программного обеспечения для платформы KUKA youBot.

![youbot](./.img/youbot.jpg)

## Подготовка рабочего окружения

Для работы вам потребуется компьютер под управлением операционной системой Ubuntu 20.04 и установленная операционная система для роботов ROS Kinetic Kame. Данная инструкция подойдет и к более старым версиям.

[Инструкция по установке Ubuntu 20.04](https://losst.ru/ustanovka-ubuntu-20-04)

[Инструкция по установке ROS](http://wiki.ros.org/noetic/Installation/Ubuntu)

## Полезные ссылки

[Кинематика и 3D модель youbot](http://www.youbot-store.com/developers/kuka-youbot-kinematics-dynamics-and-3d-model-81)

[Репозитории youbot](https://github.com/youbot)

[Введение в ROS](https://github.com/shamoleg/course)



## Установка

Создайте рабочую область ROS или пропустите данный пункт если рабочая область существует:

```console
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src/
catkin_init_workspace
cd ~/catkin_ws
catkin_make
```

Для использования созданного окружения добавьте выполнение команды в файл .bashrc

```console
echo "source ~/catkin_ws/devel/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

Установите git:

```console
sudo apt-get install git
```

Клонируйте данный репозиторий в папку рабочей области:

```console
cd ~/catkin_ws/src
git clone https://github.com/Askenavn/youbot.git
git clone https://github.com/Askenavn/mecanum_drive_controller.git
```

Клонируйте данные репозитории субмодулей:

```console
cd ~/catkin_ws/src/youbot
git submodule init
git submodule update

cd ~/catkin_ws/src/mecanum_drive_controller
git submodule init
git submodule update
```

Скачайте необходимые зависимости и соберите клонированные пакеты:

```console
cd ~/catkin_ws
rosdep install --from-paths src --ignore-src -r -y
catkin_make
```

Разрешите исполняемым файлам взаимодействовать с EtherCAT:

```console
cd ~/catkin_ws/
sudo setcap cap_net_raw+ep devel/lib/youbot_control/youbot_control
```

Установите утилиту ifconfig:

```console
sudo apt-get install net-tools
```

Узнайте название Ethernet интерфейса запустив утилиту:

```console
ifconfig
```

В приведенном на картинке примере названием является enp5s0:

![ifconfig](./.img/ifconfig.png)

Перейдите в директорию конфигурационных файлов youbot_driver/config:

```console
roscd youbot_controll/config 
```

Измените в название EthernetDevice на название Ethernet интерфейса компьютера отредактировав файл youbot-ethercat.cfg консольным текстовым редактором nano:

```console
nano youbot-ethercat.cfg
```

Конфигурационный файл должен выглядеть как на приведенной картинке ниже

![eth](./.img/eth.png)

Установите управление с помощью клавиатуры
```console
sudo apt-get install ros-noetic-teleop-twist-keyboard
```

Персональный компьютер готов к управлению мобильной платформой youbot

## Запуск мобильной платформы

Для запуска платформы последовательно выполните:
- Подключите блок питания к платформе
- Зажмите черную кнопку на верхней панели до момента когда загорится дисплей
- Зажмите черную кнопку и отпустите когда на экране загориться надпись "Motor on"
- Однократно нажмите горящую красным кнопку расположенную на манипуляторе
- Соедините персональный компьютер и мобильную платформу патч кордом (ethernet кабелем)

Мобильная платформа включена и готова к работе.

Обновите кэш разделяймых библиотек (с правами администратора обязательно):
```console
sudo ldconfig /opt/ros/noetic/lib/
```

Запустите yзел управления на персональном компьютере:
```console
roslaunch youbot_control youbot_driver_interface.launch
```

Запустите управление передвижением:
```console
rosrun teleop_twist_keyboard teleop_twist_keyboard.py
```

При необходимости переопределите название топика для управления с клавиатуры:
```console
rosrun teleop_twist_keyboard teleop_twist_keyboard.py /cmd_vel:=<new_topic_name>
```

Для остановки работы узла в выбранном окне терминала нажмите сочетание клавиш  `Ctrl + C`

Для выключения мобильной платформы зажмите черную кнопку и отпустите когда на экране загориться надпись "Switch off", экран должен погаснуть.

## Подключение к мобильному роботу по Wi-Fi через ssh
Перед процедурой подключения установите пакет openssh-server

Включите робота согласно предыдущей инструкции, и дополнительно включите внутренний компьютер: 
- Зажмите черную кнопку и отпустите когда на экране загориться надпись "PC on"

Для связи по ssh:
- Подключите компьютер или ноутбук к локальной сети робота.
- Введите в терминале команду:
```console
ssh <имя робота>@<ip адрес робота> -p<номер порта подключения>
ssh youbot@255.255.0.255 -p22 //пример подключения
```
Соединение налажено и робот готов к работе.
В терминале, где было налажено соединение, произведите запуск узла управления как в предыдущем пункте:

Обновите кэш разделяймых библиотек:
```console
sudo ldconfig /opt/ros/noetic/lib/
```
Запустите yзел управления:
```console
roslaunch youbot_control youbot_driver_interface.launch
```
Для связи с запущенным ядром ROS в терминале вашего персонального компьютера введите команды:
```console
export ROS_IP=<ip адрес робота>
export ROS_MASTER_URI=http://<ip адрес робота>:11311  #обычно данная информация выводится в терминале, где было запущено ядро
export ROS_HOSTNAME=<ip адрес вашего компьютера>
```

Ваш персональный компьютер готов к взаимодействию с узлами ROS, запущенными на внутреннем компьютере робота. При открытии нового окна в терминале - необходимо прописывать команды export заново.
