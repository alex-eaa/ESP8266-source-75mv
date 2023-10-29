## ESP8266-source-75mv
### Источник регулируемого напряжения 0-160 мВ, на базе микроконтроллера ESP8266.
---
#### Функционал устройства:
- Управление, через вебинтерфейс позволяет:
    - включать/отключать выходное напряжение;
    - регулировать величину выходного напряжения в диапазоне от 0 до 160 мВ.
- Питание от двух аккумуляторов типа 18650.
- Контроль напряжения аккумуляторов:
    - отображение в вебинтерфейсе заряда в процентах (0-100%);
    - индикация заряда с помощью красного светодиода.

##### Режимы отображения заряда батареи с помощью красного светодиода:
- светодиод светится постоянно (заряд 31%-100%);
- светодиод медленно мигает (заряд в диапазоне 10%-30%);
- светодиод быстро мигает (заряд менее 10%), необходимо обязательно зарядить.

#### Управление и настройка контроллера:
Управление кортроллером может выполняться через веб-интерфейс из любого браузера.
- контроллер может работать в режиме WIFI-точки доступа;
- контроллер может работать в режиме WIFI-клиента для подключения к роутеру;
- автозапуск в режиме WIFI-точки доступа "По умолчанию" с настройками по умолчанию (для первичной настройки устройства);

##### Страницы для управление через веб-интерфейс:
- Страница управления и настройки реле (/index.htm);
- Страница сетевых настроек (/setup.htm);
- Страница файлового менеджера для просмотра, удаления и загрузки файлов (/edit.htm);
- Страница обновления прошивки контроллера (/update.htm);

| ![Alt-текст](screnshoots/Screenshot_20231028-164141.jpg) | ![Alt-текст](screnshoots/Screenshot_20231028-164154.jpg) | ![Alt-текст](screnshoots/Screenshot_20231028-164240.jpg) | ![Alt-текст](screnshoots/Screenshot_20231028-164255.jpg) |
|:---------:|:---------:|:---------:|:---------:|

#### Используемые технологии:
- Web сервер (порт: 80) на устройстве для подключения к устройству по сети из любого web-браузера;
- Websocket сервер (порт: 81) на устройстве для обмена коммандами и данными между страницей в web-браузере и контроллером;
- mDNS-сервис на устройстве для автоматического определения IP-адреса устройства в сети;

#### Подключение ЦАП MCP4725 12-бит (1 LSB = VCC Voltage / 4096):
- OUT – A0
- GND – Gnd
- VCC – nodeMCU 3.3v
- SCL – nodeMCU GPIO5 (D1)
- SDA – nodeMCU GPIO4 (D2)

#### Подключение других элементов (для платы типа nodeMCU ESP8266):
    - GPIO2  (D4) - голубой wifi светодиод;
    - GPIO14 (D5) - красный светодиод;
    - GPIO12 (D6) - кнопка запуска с настройками сети по умолчанию;

#### Напряжение батареи: 
- верхний предел 100% - 4,2В - 780 единиц на АЦП;
- нижний предел  0%   - 3,2В - 668 единиц на АЦП.
