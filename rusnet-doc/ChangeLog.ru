ВНИМАНИЕ: этот файл может быть недостаточно свеж, смотрите ChangeLog
--------------------------------------------------------------------------
Версия 2.0
--------------
2011-09-22 erra <erra@ya.ru>
	* ircd/s_misc.c: вызов check_triggers() удалён из exit_one_client
	* ircd/s_user.c: вызов check_triggers() добавлен в m_quit
	* ircd/s_user_ext.h: удалено объявление EXTERN для check_triggers()

2011-09-20 erra <erra@ya.ru>
	* common/support.c, common/support_ext.h: добавлена функция cstrip()
			для удаления расцветок перед проверкой сообщения
			на спам
	* ircd/channel.c: вызов check_spam добавлен в m_part()
	* ircd/s_conf.c: не перекодируем ircd.conf/kills.conf, если при сборке
			не было задано DECODE_CONFIG
	* ircd/s_debug.c: STRIP_COLORS выводится как 'b' в server_opts
	* ircd/s_err.c: исправлен формат RPL_STATSTRIGGER
	* ircd/s_misc.c: вызов check_spam() добавлен в exit_one_client
	* ircd/s_serv.c: исправлен вывод /stats T
	* ircd/s_user.c: добавлена функция check_spam() для вызова из
			m_message, m_part и exit_one_client
	* support/config.h.dist: добавлен параметр сборки STRIP_COLORS,
			по умолчанию включен; добавлен параметр DECODE_CONFIG,
			по умолчанию выключен

2011-09-20 erra <erra@ya.ru>
	* ircd/s_serv.c: исправлены m_trigger/m_untrigger на предмет
			необязательности класса

2011-09-18 erra <erra@ya.ru>
	* common/match.c: bzero used instead of {0} assignment for mbs
2011-09-18 erra <erra@ya.ru>
	* common/match.c: для mbs использован вызов bzero вместо {0}
	* ircd/s_user.c: исправлено условие в check_triggers
	* ircd/s_serv.c: исправлено условие в match_trigger

2011-09-15 erra <erra@ya.ru>
	* ircd/s_serv.c: m_trigger/m_untrigger для T:lines, /stats T
	* iserv/iserv.c: добавлена обработка строк T:

2011-09-14 erra <erra@ya.ru>
	* */*: удалено #ifdef RUSNET_RLINES (используется всегда)
	* common/match.c: задано MBS_VALUE для инициализации mbtstat_t
	* common/numeric_def.h, common/struct_def.h, ircd/s_err.c,
		ircd/s_user.c: добавлен режим пользователя +c (бесцветный)
	* ircd/channel.c: не отправляем +R старым серверам; восстановлено
			назначение режимов (влияло на режимы с параметрами)

2011-09-13 erra <erra@ya.ru>
	* common/send.c: удалён код запрета цветных сообщений (содержит ошибки)
	* ircd/channel.c: подстановка сообщения в PART для каналов в режиме +c
	* ircd/s_user.c: исправлена сборка с WHOIS_NOTICES

2011-09-12 erra <erra@ya.ru>
	* common/collsyms.h, common/match.c, common/sm_loop.c: для проверки
			соответствия строк шаблону использован мультибайтный
			код из GNU bash
	* support/Makefile.in: поправлены зависимости для match.o

2011-09-11 erra <erra@ya.ru>
	* common/conversion.c: определение ICONV_CHAR поправлено для FreeBSD
	* common/os.h: #include <wctype.h> для FreeBSD
	* ircd/ircd.c: поправлено использование ttyname (зачистка кода)
	* ircd/res_init.c: отменено переопределение _D_UNUSED_ (зачистка кода)
	* ircd/s_auth.c: исправлена опечатка в отладочном коде (зачистка кода)
	* ircd/s_conf.c: отменено неиспользуемое преобразование
			типа (зачистка кода)
	* ircd/s_id.c: преобразование беззнакового типа (зачистка кода)
	* ircd/s_serv.c: функция check_tconfs удалена под #if 0
			как неиспользуемая
	* support/configure, support/configure.in: для FreeBSD добавлен
			параметр -I/usr/local/include для определения idna.h

2011-09-09 erra <erra@ya.ru>
	* common/conversion.c, common/dbuf.c, common/dbuf_def.h,
		common/dbuf_ext.h, common/match.c, common/parse.c,
		common/send.c, common/struct_def.h, common/sys_def.h,
		iauth/a_defines.h, iauth/a_io.c, iauth/a_io_ext.h,
		iauth/a_log.c, iauth/a_log_ext.h, iauth/iauth.c,
		iauth/mod_dnsbl.c, iauth/mod_lhex.c, iauth/mod_socks.c,
		iauth/mod_webproxy.c, ircd/channel.c, ircd/hash.c,
		ircd/hash_ext.h, ircd/ircd.c, ircd/ircd_signal.c, ircd/res.c,
		ircd/res_init.c, ircd/res_mkquery.c, ircd/res_mkquery_ext.h,
		ircd/s_auth.c, ircd/s_bsd.c, ircd/s_bsd_ext.h, ircd/s_conf.c,
		ircd/s_conf_ext.h, ircd/s_id.c, ircd/s_misc.c, ircd/s_serv.c,
		ircd/s_serv_ext.h, ircd/s_service.c, ircd/s_service_ext.h,
		ircd/s_user.c, ircd/s_user_ext.h, ircd/s_zip.c,
		ircd/s_zip_ext.h, ircd/ssl.c, ircd/ssl.h, ircd/whowas.c,
		iserv/iserv.c, rusnet/rusnet_cmds.c, rusnet/rusnet_cmds_ext.h,
		rusnet/rusnet_virtual.c: массовая зачистка кода
	* support/configure, support/configure.in: для gcc добавлен параметр
			сборки -Wextra
	* support/Makefile.in: удалён клиент

2011-09-08 erra <erra@ya.ru>
	* common/match.c: параметры вызова match() обрабатываются idn_encode,
		если задано USE_LIBIDN
	* common/send.c: скорректирован вызов bzero по массиву sentalong
	* common/support.c, ircd/res_comp.c: зачистка кода, порождающего
		предупреждения при сборке

2011-09-07 erra <erra@ya.ru>
	* common/send.c: подмена цветного сообщения в QUIT для каналов
			в режиме +c
	* ircd/res.c: защита idn от некорректного высвобождения; ещё пару строк
			отладочной информации; приведение имени в нижний регистр
			для преобразования IDN (иначе не работает)
	* ircd/s_bsd.c: защита от ошибки сегментирования в отсутствие строки M:

2011-09-06 erra <erra@ya.ru>
	* ircd/s_user.c: режим +I для пользователя разрешён только
			посредством SVSMODE

2011-09-02 erra <erra@ya.ru>
	* common/conversion.c, common/conversion.h.in, common/match.c,
		common/match_ext.h, common/packet.c, common/send.c,
		common/support.c, common/support_ext.h, common/struct_def.h,
		common/sys_def.h, contrib/ircdwatch/ircdwatch.c,
		iauth/a_conf.c, iauth/a_io.c, iauth/a_log.c, iauth/mod_dnsbl.c,
		iauth/mod_pipe.c, iauth/mod_rfc931.c, iauth/mod_socks.c,
		iauth/mod_webproxy.c, ircd/channel.c, ircd/channel_ext.h,
		ircd/chkconf.c, ircd/config_read.c, ircd/hash.c, ircd/ircd.c,
		ircd/res.c, ircd/s_auth.c, ircd/s_bsd.c, ircd/s_bsd_ext.h,
		ircd/s_conf.c, ircd/s_conf_ext.h, ircd/s_id.c, ircd/s_misc.c,
		ircd/s_user.c, ircd/s_user_ext.h, ircd/s_zip.c,
		ircd/s_zip_ext.h, ircd/version.c.SH.in, ircd/whowas.c,
		ircd/whowas_ext.h, iserv/i_io.c, iserv/i_log.c,
		iserv/iserv.c, iserv/iserv.h, rusnet/rusnet_cmds.c,
		rusnet/rusnet_virtual.c: зачистка кода (сборка без
			лишних сообщений)
	* ircd/s_user.c: сервисам разрешены сообщения пользователям,
			включившим режим +R (сообщения только от
			зарегистрированных пользователей)
	* support/config.h.dist, support/Makefile.in: убраны дубликаты макросов
	* support/configure, support/configure.in: для компилятора gcc параметры
			сборки изменены на -Wall -funsigned-char

2011-09-02 erra <erra@ya.ru>
2011-09-01 erra <erra@ya.ru>
	* common/numeric_def.h, ircd/s_err.c: добавлен RPL_IDENTIFIED
	* ircd/s_user.c: вывод RPL_IDENTIFIED добавлен в send_whois; вывод "I"
		добавлен в who_one
	* support/configure, support/configure.in: при подключении libiconv.a
		принято во внимание amd64; добавлено переопределение CPPFLAGS
		при проверке IDN

2011-08-31 erra <erra@ya.ru>
	* common/send.c, common/send_ext.h: экспорт dead_link для s_bsd.c
	* ircd/s_bsd.c: если получен ECONNRESET, вызываем dead_link; отменён
		вызов enable_extended_FILE_stdio для amd64

2011-08-30 erra <erra@ya.ru>
	* support/configure, support/configure.in: исправлен тест локали

2011-08-29 erra <erra@ya.ru>
	* ircd/s_conf.c: DEFAULT_CHARSET применяется к портам, кодировка для
			которых не указана
	* ircd/s_serv.c: добавлена проверка на NULL в функции report_listeners
			во избежание SIGSEGV в /stats P при наличии портов
			с неверно заданной кодировкой
	* ircd/s_user.c: поправлено использование макроса DupString
	* support/config.h.dist: добавлен DEFAULT_CHARSET

2011-08-14 erra <erra@ya.ru>
	* rusnet/rusnet.h: вновь добавлено объявление функции rusnet_zmodecheck,
			требуемое в ircd/s_user.c

2011-08-05 erra <erra@ya.ru>
	* ircd/s_user.c: проверка на R-mode передвинута выше

2011-08-04 erra <erra@ya.ru>
	* common/send.c: размер буфера перекодировки увеличен в целях
			безопасности

2011-08-03 erra <erra@ya.ru>
	* ircd/s_misc.c: вызов conv_free_conversion перемещён из exit_one_client
			в exit_client в блок MyConnect(sptr) во избежание
			ложных вызовов

2011-08-02 erra <erra@ya.ru>
	* ircd/list.c: исправлено обрамление #ifdef HOLD_ENFORCED_NICKS
			для cptr->held

2011-07-29 erra <erra@ya.ru>
	* common/numeric_def.h, common/struct_def.h, ircd/s_user.c,
		ircd/s_err.c, support/config.h.dist: добавлен параметр
			HOLD_ENFORCED_NICKS, чтобы дать возможность настраивать
			удержание ников

2011-07-28 erra <erra@ya.ru>
	* rusnet/rusnet_cmds.c: автоматическая замена "translit" на "ascii"

2011-07-26 erra <erra@ya.ru>
	* ircd/s_misc.c: удалён неиспользуемый код libidn (вызывал ошибки)

2011-07-25 erra <erra@ya.ru>
	* ircd/channel.c, ircd/s_user.c: исправлен вызов is_chan_anyop

2011-07-24 erra <erra@ya.ru>
	* ircd/channel.c, ircd/hash.c: удалены устаревшие части кода

2011-07-23 erra <erra@ya.ru>
	* common/msg_def.h, common/parse.c, ircd/channel.c,
		ircd/channel_ext.h: добавлен NTOPIC (синхронизация топика при
			соединении сервера с сетью или другим сервером)
	* ircd/channel.c, ircd/channel_ext.h, ircd/s_serv.c:
			добавлена функция send_channel_topic() (для отправки
			команды NTOPIC на нужные серверы)
	* ircd/channel.c: is_chan_halfop заменена на is_chan_anyop

2011-07-19 erra <erra@ya.ru>
	* ircd/ircd.c: добавлен вызов setlocale(LC_MESSAGES, SYS_LOCALE)
	* support/config.h.dist: добавлено определение для SYS_LOCALE (для
			настройки LC_TIME и LC_MESSAGES, по умолчанию C)

2011-07-18 erra <erra@ya.ru>
	* common/numeric_def.h, ircd/s_err.c: добавлена ошибка ERR_NICKTOOFAST
	* common/struct_def.h: для локальных клиентов добавлен маркер задержки;
		указатель на кодировку перемещён в часть для локальных клиентов
	* common/send.c: исправлена инициализация клиента; переработана функция
		send_message()
	* ircd/list.c: добавлена инициализация для held; conv перемещено
	* ircd/s_user.c: для форсированных ников реализовано удержание и ошибка
		смены ника ERR_NICKTOOFAST

2011-07-17 erra <erra@ya.ru>
	* common/msg_def.h, common/parse.c, rusnet/rusnet_cmds.c,
		rusnet/rusnet_cmds_ext.h: добавлены m_svsmode и m_svsnick

2011-07-08 erra <erra@ya.ru>
	* ircd/s_user.c: в send_umode_out() блокирована отправка новых режимов
			на старые серверы

2011-07-02 erra <erra@ya.ru>
	* ircd/res.c: отключено преобразование IDN для T_PTR
	* ircd/s_bsd.c: добавлена функция send_hostp_to_iauth()

2011-07-01 erra <erra@ya.ru>
	* support/configure, support/configure.in: проверка libidn исправлена
			для SunOS

2011-06-28 erra <erra@ya.ru>
	* */*: удалён параметр RUSNET_IRCD (стал обязательным), код очищен от
		устаревших проверок и условий
	* common/struct_def.h: добавлен режим идентифицированного
		пользователя (+I), а также доступ только для зарегистриованных
		ников в приват и на канал (режимы пользователя и канала +R);
		добавлены флаги SV_RUSNET1 и SV_RUSNET2; удалён флаг SV_29,
		а также остальные устаревшие флаги
	* ircd/channel.c: добавлен режим MODE_RECOGNIZED (+R)
	* ircd/res.c: добавлена поддержка libidn
	* ircd/s_user.c: добавлены флаги режимов пользователя
		FLAGS_IDENTIFIED (+I) и FLAGS_RECOGNIZED (+R)
	* support/configure.in: добавлена проверка для libidn
	* support/setup.h.in: добавлен параметр USE_LIBIDN

Версия 1.5.16
--------------
2011-06-22 erra <erra@ya.ru>
	* configure: исправлен PATH для SunOS; echo -e используется только
		для FreeBSD
	* common/conversion.c: использован HAVE_ICONV_TRANSLIT
	* ircd/ircd.s: добавлена переменная окружения LOCPATH для использования
		собственной локали на Linux
	* support/Makefile.in: добавлена установка собственной локали на Linux
	* support/configure.in: исправлены тесты iconv, добавлено сообщение
			для IPv6; исправлены подстановки include и lib для
			zlib и SSL
	* support/setup.h.in: добавлены USE_VHOST6 и LOCALE_PATH

2011-06-20 erra <erra@ya.ru>
	* configure: тесты локали и параметр --no-rusnet удалены
	* contrib/uk_UA: правильная локаль для Linux
	* support/locale_tests: тестовые строки для проверки локали
	* support/setup.h.in: добавлено HAVE_ICONV_TRANSLIT
	* support/configure.in: переработаны проверки zlib, SSL и iconv;
			добавлены тесты локали

Версия 1.5.15
--------------
2011-06-17 erra <erra@ya.ru>
	* ircd/s_user.c: проверка канала на режим +z отменена для коллизий
		ников, чтобы избежать рассинхронизации вследствие защиты
		ников сервисами
	* rusnet/rusnet_misc.c: rusnet_isagoodnickname() переименована
		в rusnet_zmodecheck() и оптимизирована
	* support/config.h.dist: добавлены ошибочно удалённые сервисные ники

Версия 1.5.14
--------------
2011-02-11 erra <erra@ya.ru>
	* ircd/s_user.c: исправлен VHOST для адресов IPv6;
			устранена утечка памяти в m_nick()
	* tkserv: удалён (устарел, заменён на iserv)

2011-02-10 erra <erra@ya.ru>
	* ircd/s_misc.c: исправлено переполнение стека в вызове exit_client()
			по наводке denk

2011-01-10 denk <denis@tambov.ru>
	* ircd/s_user.c: исправлена утечка памяти в ISTAT

2010-11-04 denk <denis@tambov.ru>
	* ircd/s_bsd.c, ircd/s_conf.c, ircd/s_misc.c, ircd/s_serv.c,
		ircd/s_user.c, iserv/i_io.c, rusnet/rusnet.h,
		support/configure.in: исправлена поддержка IPv6

Версия 1.5.13
--------------
2010-10-30 erra <erra@ya.ru>
	* support/configure.in: исправлено местонахождение OpenSSL
		по умолчанию; исправлен тест на альфа/бета версию

2010-10-27 LoSt <andrej@rep.kiev.ua>
	* configure: добавлена проверка локали и параметр --no-rusnet
	* common/conversion.h.in, support/configure.in: файл conversion.h
		теперь создаётся из прототипа; добавлена поддержка для
		внешней библиотеки libiconv, для Solaris она должна быть
		подключена статически
	* common/fileio.c, common/fileio_ext.h, ircd/chkconf.c, ircd/s_bsd.c,
		iserv/i_externs.h, support/Makefile.in, support/configure.in:
		исправлена работа с ограничением на число файловых дескрипторов
		в Solaris
	* support/configure.in: исправлена проблема с новыми версиями autoconf

2010-10-25 denk <denis@tambov.ru>
	* ircd/s_conf.c: исправлен размер адреса IPv6 для check_tlines()


Версия 1.5.12
-------------
2010-09-10 LoSt <andrej@rep.kiev.ua>
	* common/common_def.h: добавлена подстановка towupper() для систем
		с плохой поддержкой UTF8
	* common/conversion.c: вызов toupper() заменён на вызов towupper()
		для устранения проблем с многобайтными символами
	* support/configure.in, support/configure, support/setup.h.in:
		добавлено обнаружение towupper()

Версия 1.5.11
-------------
2010-09-08 erra <erra@ya.ru>
	* iserv/iserv.c: исправлен опечатка (вызов fopen)

2010-09-08 LoSt <andrej@rep.kiev.ua>
	* ircd/s_bsd.c, ircd/s_conf.c, ircd/s_serv.c: исправлено чтение
		конфигурации для устранения ошибки сегментирования вследствие
		пробемных строк P:
	* ircd/s_misc.c: устранён вызов realloc для случая использования
		информации пользователя по умолчанию в функции
		set_internal_encoding()

Версия 1.5.10
-------------
2010-07-29 erra <erra@ya.ru>
	* common/support_ext.h, common/support.c: удалена функция dgets()
		как устаревшая (заменена на fgets)
	* ircd/chkconf.c: удалены mystrinclude() и simulateM4Include()
	* ircd/config_read.c: включение файла fileio.c заменено на fileio.h
	* ircd/s_misc.c: функция dgets() заменена на fgets() в read_motd() и
		read_rmotd()
	* ircd/fileio.c: перенесен в common/ (используется в iserv)
	* ircd/fileio.h: переименован в common/fileio_ext.h (используется
		в iserv)
	* ircd/s_externs.h: добавлен fileio_ext.h
	* iserv/i_externs.h: добавлен fileio_ext.h
	* iserv/iserv.c: функция dgets() заменена на fgets()
		в flushPendingLines()
	* support/Makefile.in: добавлена компиляция fileio.c

2010-07-27 erra <erra@ya.ru>
	* ircd/s_serv.c: /stats P стал доступен для обычных пользователей

2010-07-26 LoSt <andrej@rep.kiev.ua>
	* ircd/channel.c: исправлен вызов функции, приводивший к ошибке
		сегментирования по команде /rehash при смене внутренней
		кодировки ircd

Версия 1.5.9
-------------
2010-07-26 erra <erra@ya.ru>
	* ircd/s_conf.c: исправлена опечатка
	* support/configure: переменная enableval в проверке iconv отменена
			(исключено влияние на проверку ipv6)

2010-07-26 kmale <>
	* support/configure.in: исправлена обработка переменной enableval
			в проверке ipv6

Версия 1.5.8
-------------
2010-07-22 erra <erra@ya.ru>
	* ircd/s_serv.c: в вывод /stats P добавлена кодировка (для сборки
			с iconv)
	* ircd/s_conf.c: блокирован пропуск строк CONF_RLINE в присвоение
			кодировки
	* ircd/s_bsd.c: поправлен #ifdef в функции add_listener()
	* ircd/s_misc.c, rusnet-doc/INSTALL: исправлены опечатки

Версия 1.5.7
-------------
2010-07-22 denk <denis@tambov.ru>
	* rusnet/rusnet.h, rusnet/rusnet_virtual.c: обратка строк F:
		параметризована для IPv6

Версия 1.5.6
-------------
2010-07-21 LoSt <andrej@rep.kiev.ua>
	* common/conversion.c, support/configure, support/configure.in,
		support/setup.h.in: добавлено определение TRANSLIT_IGNORE
				для управления порядком параметров iconv

Версия 1.5.5
-------------
2010-07-20 denk <denis@tambov.ru>
	* iserv/iserv.c, iserv/i_io.c: использован определяемый разделитель
		полей в ircd.conf для IPv6

Версия 1.5.4
-------------
2010-07-12 LoSt <andrej@rep.kiev.ua>
	* support/configure, support/configure.in: исправлен тест
		транслитерации
	* common/conversion.c: исправлен вызов транслитерации

Версия 1.5.3
-------------
2010-07-09 erra <erra@ya.ru>
	* common/support.c: вывод версии переработан. Добавлен вывод
			локали и кодировки для сборки с поддержкой Unicode

Версия 1.5.2
-------------
2010-07-06 
	* common/match.c: сравнение шаблонов поправлено для случая \\*

Версия 1.5.1
-------------
2010-07-02 LoSt <andrej@rep.kiev.ua>
	* common/support.c, common/support_ext.h, ircd/channel.c,
		ircd/s_conf.c, ircd/s_serv.c, ircd/s_service.c,
		ircd/s_user.c , ircd/whowas.c , ircd/whowas_def.h:
		Поправлено обрезание информации о пользователе
	* common/conversion.c: отменено преобразование спецсимволов
		согласно RFC 2822 ({}| более не равны []\) с целью
		совместимости с RusNet-1.4

Версия 1.5.0
-------------
2009-11-27 LoSt <andrej@rep.kiev.ua>
	* common/struct_def.h: размер буфера увеличен для многобайтных кодировок

1.5.0p20
2009-10-19 erra <erra@ya.ru>
	* common/bsd.c, common/bsd_ext.h: устарели и, следовательно, удалены
	* common/send_ext.h: удалено устаревшее объявление EXTERN
			функции deliver_it
	* ircd/s_user.c: серверы, использующие только IPv4, должны нормально
			обращаться с адресами IPv6; ifdef RUSNET_RLINES
			в send_whois() извлечён из не имеющего к нему отношения
			ifdef USE_OLD8BIT, переработана проверка управляющих
			символов

1.5.0p19
2009-09-25 erra <erra@ya.ru>
	* iauth/a_io.c: удалена переменная minus_one

2009-09-21 erra <erra@ya.ru>
	* ircd/s_user.c: патч для IPv6 и режима +X by denk
	* ircd/s_bsd.c, ircd/s_conf.c, common/os.h: переменная minus_one
			удалена by denk

1.5.0p18
2009-07-21 erra <erra@ya.ru>
	* rusnet/rusnet_virtual.c: два вызова memset() заменены одним вызовом
		bzero() (решение проблемы переполнения буфера в 64-битных
		архитектурах)

2009-06-02 erra <erra@ya.ru>
	* ircd/s_user.c: вызов exit_client() для ложной коллизии

2009-05-22 erra <erra@ya.ru>
	* ircd/s_serv.c: исключаем проверки, бесполезные для RusNet ircd
	* ircd/list.c: оптимизировано выделение клиентской структуры
	* ircd/channel.c: ошибки NJOIN отправляем на &ERRORS не только
		в отладочном режиме

1.5.0p17
2008-10-10 erra <erra@ya.ru>
	* ircd/s_conf.c: восстановлен предыдущий вариант detach_conf()
	* ircd/s_misc.c: исключена двойная проверка MyConnect(sptr)

1.5.0p16
2008-10-03 erra <erra@ya.ru>
	* ircd/s_bsd.c: catching bugs: verify strerror_ssl against "Success".

2008-09-17 erra <erra@ya.ru>
	* ircd/s_conf.c: code clearance: do not leave unfreed aconf when
		there are clients; copy clients from aconf to bconf for
		SSL connections

1.5.0p15
2008-09-12 erra <erra@ya.ru>
	* *: 1.4.x branch merged.

2008-09-12 LoSt <andrej@rep.kiev.ua>
	* ircd/s_user.c: server flags string len increased by one to pass
		the complete flag set

1.5.0p12
2007-02-06 LoSt <andrej@rep.kiev.ua>
	* ircd/s_err.c: changed RPL_CODEPAGE message (it was requested by
	  alexey@kvirc.ru).
	* ircd/channel.c: fixed channel modes that were lost in transit.

1.5.0p11
2006-04-27 LoSt <andrej@rep.kiev.ua>
	* common/conversion.c: fixed coredump due to not-nullified prev member
	  in conversion_t structure.

2006-03-17 LoSt <andrej@rep.kiev.ua>
	* common/channel.c, common/hash.c: fixed channel name length (might be
	  in bytes instead of chars), got NJOIN protocol errors due to of that.
	* common/channel.c: channel name might be changed in get_channel() so
	  fixed it in m_join().

2006-03-09 LoSt <andrej@rep.kiev.ua>
	* common/packet.c: implemented validation of UTF input since we cannot
	  trust users, they may send invalid UTF strings. :(

2006-02-20 LoSt <andrej@rep.kiev.ua>
	* common/match*, common/common_def.h, ircd/channel.c, ircd/ircd.c:
	  rewritten validation via implemented validtab[].
	* common/send.c, support/Makefile.in, support/configure.in: cosmetic
	  fixes.

2006-02-14 LoSt <andrej@rep.kiev.ua>
	* ircd/channel.c: fixed channel invalidation when internal charset is
	  CHARSET_8BIT.
	* ircd/s_conf.c, ircd/s_bsd.c, ircd/s_bsd_ext.h: listening ports may
	  be reused with the same port but changed P:line or p:line on REHASH.

1.5.0p8
2006-01-26 LoSt <andrej@rep.kiev.ua>
	* common/conversion.c: fixed bug with state of mbtowc().
	* ircd/channel.c: fixed debug message in get_channel().
	* ircd/ircd.c, ircd/ircd_s.h, ircd/s_conf.c: close and open debugfile
	  on -SIGHUP to enable log rotation.

2006-01-18 LoSt <andrej@rep.kiev.ua>
	* common/common_def.h: don't use isalnum() when strict locales asked
	* ircd/s_serv.c: double charset conversion removed

1.5.0p7
2006-01-17 erra <erra@rinet.ru>
	* *: add-ons from LoSt imported
	* ircd/res.c: JAIL_RESOLVER_HACK narrowed to INET4. Currently
		this version won't compile with --enable-ipv6
	* common/conversion.c: minor syntax fix to MyFree() call

2006-01-16 erra <erra@rinet.ru>
	* *: redone merge into 1.4.3 release

1.5.0p6
2005-06-20 LoSt <andrej@rep.kiev.ua>
	* *: merged with 1.4.3 version - it's need to be checked now!

1.5.0p5
2005-06-18 LoSt <andrej@rep.kiev.ua>
	* *: added USE_OLD8BIT definition to easy merge with 1.4.3 versions.
	  deprecated files are findable by checking configure.in and
	  Makefile.in for @oldconvobjs@

1.5.0p4
2005-02-14 LoSt <andrej@rep.kiev.ua>
	* *: bugfixes from 1.4.2_3

2004-11-30 LoSt <andrej@rep.kiev.ua>
	* common/match.c: fixed bug with '?' in hostmask - it's not byte
	  in multibyte

2004-11-06 LoSt <andrej@rep.kiev.ua>
	* ircd/channel.c: fixed SIGBUS with unistrcut() on /part

1.5.0p3
2004-11-05 LoSt <andrej@rep.kiev.ua>
	* support/config.h.in, ircd/ircd.c, ircd/s_bsd.c, ircd/s_conf.c, ircd/s_debug.c, ircd/s_misc.c, ircd/s_user.c, ircd/ircd_signal.c, common/send.c, common/send_ext.h:
	  rewritten all logging mechanics, introduced g: lines for logging

1.5.0p2
2004-11-04 LoSt <andrej@rep.kiev.ua>
	* ircd/ircd.c, ircd/ircd_ext.h, ircd/s_conf.c, ircd/s_misc.c, ircd/s_serv.c:
	  added charset selection for ircd.conf and ircd.mord
	* ircd/s_debug.c: updated VERSION reply with new configuration options

2004-11-01 LoSt <andrej@rep.kiev.ua>
	* ircd/channel.c, ircd/s_user.c, common/support.c, common/support_ext.h:
	  fixed bug with topic/reasons length (i.e. in chars)
	* rusnet/rusnet_cmds.c: change nick on client side after /codepage
	* common/msg_def.h, common/parse.c: added command /charset in addition
	  to /codepage

1.5.0p1
2004-10-27 LoSt <andrej@rep.kiev.ua>
	* ?: fixed bug with limitation of 512 bytes per send everywhere (RFC
	  says: chars)
	* *: moved conversion* to common/ directory and get rid of unused files

2004-10-22 LoSt <andrej@rep.kiev.ua>
	* common/send.c, conversion.c, support/config.h.dist: fixed
	  "No text to send" from 8bit server links, added DO_TEXT_REPLACE mode

2004-10-05 LoSt <andrej@rep.kiev.ua>
	* *: updated to 1.4.2pre_20

1.5.0p0
2003-09-29 LoSt <andrej@rep.kiev.ua>
	* *: added UTF-8 negotiation to server link protocol
	   - get rid of rest of rusnet.conf and translations tables
	   - changed chname for aChannel to dynamically allocated
	   - added uppercase fields to structures for fast nicks comparisons

Version 1.4.12
-------------
2008-09-08 erra <erra@ya.ru>
	* ircd/s_bsd.c: code clearance: check results for set_sock_opts for
		SSL connections; call close_conection() instead of repeating
		portions of code.

Version 1.4.11
-------------
2008-08-25 erra <erra@ya.ru>
	* ircd/channel.c: code clearance: return NULL in get_channel() when
		CREATE was not defined; free newly created channel in m_join()
		when can_join() failed; do not attempt to create channel if
		IsChannelName() fails in m_njoin()
	* ircd/s_bsd.c: code clearance: close fd (optionally) and free cptr
		when bind failed. While it should never happen, the action
		must be proper, anyway.
	* ircd/ircd.c, irc/s_serv.c, ircd/s_user.c: SetRMode() call moved
		to do_restrict
	* ircd/s_conf.c: do not double '%' prefix when R-line is renewed;
		set prefix only when NO_PREFIX is unset

Version 1.4.10
-------------
2008-07-02 skold <skold@anarxi.st>
	* ircd/s_user.c, s_err.c, common/numeric_def.h: test links KILL patch
		removed from code. Surely you are not saying you can undo
		kills on the server that issued them without having desync.
		You can't stop the damage already been done on the test server.
		The only solution here is to squit the test server in response
		to remote kill
	* ircd/s_serv.c, s_err.c, common/numeric_def.h: SQUIT disabling logic
		has been re-written so it wouldn't cause servers desync across
		network

Версия 1.4.9
-------------
2008-06-19 erra <erra@ya.ru>
	* ircd/s_serv.c: команда SQUIT сделана недоступной для тестовых
		серверов
	* ircd/s_user.c: команда KILL сделана недоступной для тестовых
		серверов
	* common/numeric_def.h: добавлены ERR_TESTCANTKILL и ERR_TEST_CANTSQUIT
	* doc/example.conf: добавлено описание признака тестового сервера
		в описание строк N:

Версия 1.4.8
-------------
2008-05-16 erra <erra@ya.ru>
	* ircd/channel.c: канал &ISERV закрыт для доступа всем, кроме
		операторов сети

2008-04-21 erra <erra@ya.ru>
	* ircd/whowas.c: действительно устранён выход указателя за пределы
		массива в функции release_locks_byserver()

Версия 1.4.7
-------------
2008-03-31 Adel <adel.abouchaev@gmail.com>
	* ircd/whowas.c: устранён выход указателя за пределы массива
		в функции release_locks_byserver()

Версия 1.4.6
-------------
2008-03-04 erra <erra@ya.ru>
	* ircd/s_user.c: пользователям запрещено получать +x в момент
		регистрации в сети, если задано NO_DIRECT_VHOST; запрещены
		символы шаблонов в идентификаторах пользователей ('?' и '*')

Версия 1.4.5
-------------
2008-02-04 erra <erra@ya.ru>
	* ircd/s_misc.c: устранена ошибка переполнения буфера в функциях
		get_client_name и get_client_host (cptr->auth)

Версия 1.4.4
-------------
2007-09-13 skold <skold@anarxi.st>
	* ircd/channel.c: метод m_invite() теперь недоступен для пользователей
		в режиме +b
	* support/configure.in: обнаружение zlib переделано,
		обнаружение openssl улучшено

Версия 1.4.3
-------------
1.4.3_028 (R1_4_3_028)
2006-02-10 cj <cj@deware.com>
	* ircd/s_user.c: пользовательский режим +R переименован в +b
	* ircd/s_conf_ext.h: зачистка find_kill()
	
1.4.3_027 (R1_4_3_027)
2006-01-20 erra <erra@rinet.ru>
	* ircd/channel.c: исправлена ошибка оповещения о снятии пользователем
		статуса оператора с себя: is_chan_op() вызывался слишком
		поздно (ошибка появилась с режимом полуоператора 2005-08-24)
	* ircd/s_err.c: пользовательский режим +R был ошибочно записан в
		режимы каналов

1.4.3_026 (R1_4_3_026)
2006-01-20 erra <erra@rinet.ru>
	* ircd/channel.c: длина mbuf в m_njoin() увеличена на 1 для
		полуоператоров; режим полуоператора более не имеет приоритета
		над режимом оператора

2006-01-19 erra <erra@rinet.ru>
	* ircd/channel.c: сообщать ник/канал при возникновении ошибки
		протокола NJOIN

1.4.3_025 (R1_4_3_025)
2006-01-09 cj <cj@deware.com>
	* ircd/s_conf.c: исправление переполнения буфера при неполном
		синтаксисе строк K/R/E

1.4.3_024 (R1_4_3_024)
2005-12-26 slcio <silence@irc.run.net>
	* common/common_def.h: поправка к валидации украинских символов в
		isvalid()

1.4.3_023 (R1_4_3_023)
2005-12-24 slcio <silence@irc.run.net>
	* ircd/s_serv.c: транзитные строки K/R на серверах, не подпадающих
		под шаблон, тем не менее применялись

1.4.3_022
2005-12-20 erra <erra@rinet.ru>
	* ircd/ssl.c: идиотское перепутывание порядка аргументов в вызове
			strncpy() исправлено

2005-12-20 cj <cj@deware.com>
	* ircd/ssl.c: добавлена очистка очереди ошибок SSL

1.4.3_021 (R1_4_3_021 RC6)
2005-12-19 erra <erra@rinet.ru>
	* common/support.c: вызов strcpy() заменен на strncpy() в myctime()

2006-01-20 lost <LoSt@lucky.net>
	* ircd/s_misc.c: неверный порядок замены \r и \n на \0 исправлен

1.4.3_021 (R1_4_3_021 RC6)
2005-12-18 cj <cj@deware.com> (from skold)
	* common/msg_def.h, parse.c, ircd/s_serv.c, s_serv_ext.h: команды
			протокола EOB/EOBACK для совместимости с будущими
			релизами

2005-12-18 cj <cj@deware.com>
	* common/send.c: не считать ошибкой отсутствие лог-файла и создавать
			его

2005-12-16 erra <erra@rinet.ru>
	* configure: исправлена передача параметров в основной configure.
	* common/support.c: мелкое исправление к чтению PATCHLEVEL
			импортировано из версии 1.5.0
	* ircd/s_conf.c: вызов dgets() удалён как устаревший

2005-12-16 cj <cj@deware.com> (from Umka)
	* ircd/ircd.c: исправление для использования #ifdef IRC_UID

2005-12-14 erra <erra@rinet.ru>
	* ircd/s_user.c: выводить клиенту его настоящий хост в send_whois()
			когда включено шифрование

2005-12-13 erra <erra@rinet.ru>
	* ircd/s_user.c: форсировано шифрование для доменов второго уровня,
			содержащих \d+-\d+ inside - для домашних сетей

1.4.3_020 (R1_4_3_020 RC6)
2005-12-08 slcio <silence@irc.run.net>
	* support/configure.in, configure: правка для Solaris-2.8 (SunOS 5.8)
			на предмет использования libcrypt вместо openssl
			libcrypto.
	* common/support.c: возможность добавлять чужие версии в строку версии
	* ircd/s_serv.c: вывод 'L' в m_stats() перемещен в 'N', L оставлено
			для статистики по логам

2005-12-04 slcio <silence@irc.run.net>
	* support/config.h.dist, ircd/s_serv.c: USE_TKSERV отделено от
			USE_SERVICES, чтобы позволить использование USE_SERVICES
			без tkserv.
	* ircd/channel.c, support/config.h.dist: добавлено определение
			JOIN_NOTICES для контроля попыток входа на каналы
			локальных клиентов
	* ircd/s_misc.c: в лог &OPER добавлены обрывы связи локальных клиентов
	* ircd/ircd.c, ircd/s_serv.c, ircd/s_user.c: устранено обрезание
			причины kline/rline

2005-11-14 erra <erra@rinet.ru>
	* ircd/channel.c: разрешил операторам канала убирать +o/+h в случае
			когда оператор ещё и полуоператор.

1.4.3_019 (R1_4_3_019 RC5)
2005-11-13 cj <cj@deware.com>
	* ircd/channel.c: Исправлена функция m_invite(), что позволит
			Сервисам посылать приглашения на каналы. Обновление
			до этой версии ВАЖНО в свете совместимости.

1.4.3_018 (R1_4_3_018 RC4)
2005-11-03 cj <cj@deware.com>
	* iserv/i_log.c: Изменено макро с LOG_DAEMON на LOG_FACILITY в
			функции openlog() для syslog.

2005-11-02 cj <cj@deware.com> (from slcio <silence@irc.run.net>)
	* ircd/s_bsd.c: некорректное использование free_server() в
			функции connect_server(): список подключенных серверов
			не обновлялся, когда соединение обрывалось. Это могло
			приводить к бесконечным циклам в функции m_server().

2005-11-01 cj <cj@deware.com>
	* ircd/ssl.c, s_bsd.c, ircd.c, ircd_ext.h: сообщения о разрыве
			соединения заменены сообщениями SSL, если определено
			IsSSL. Небольшое исправление в ssl_fatal() дает нам
			шанс исправить случайные отключения SSL-соединений.

2005-11-01 erra <erra@rinet.ru>
	* ircd/s_conf.c: проверка доменных масок в строках N: и отмена, если
			маска более 4 (вероятно, номер порта)

1.4.3_017 (R1_4_3_017 RC3)
2005-10-21 erra <erra@rinet.ru>
	* common/send.c: замена присвоения на вызов dup() решает проблемы
			с дальнейшим закрытием файловых дескрипторов

1.4.3_016 (R1_4_3_016 RC2)
2005-10-13 cj <cj@deware.com>
	* ircd/s_user.c: исправлено использование макро  ClearWallops(x),
			добавлено ограничение пользовательского режима +w
			только для операторов

2005-10-13 erra <erra@rinet.ru>
	* ircd/s_serv.c: вызовы sendto_ops_butone заменены на вызовы
			sendto_serv_butone для WALLOPS_TO_CHANNEL
	* common/send.c, common/send_ext.h: функция sendto_ops_butone
			исключена для WALLOPS_TO_CHANNEL
	* common/struct_def.h, common/numeric_def.h, ircd/s_err.c,
	  ircd/s_user.c: сброс пользовательского режима +w с сообщением
			про канал &WALLOPS под WALLOPS_TO_CHANNEL

2005-10-12 erra <erra@rinet.ru>
	* common/send.c: sendto_ops_butone возвращена к предыдущему виду и
			несколько оптимизирована
	* ircd/s_serv.c: вызовы sendto_ops_butone сопровождаются вызовами
			sendto_flag(SCH_WALLOPS...) для WALLOPS_TO_CHANNEL

1.4.3_015 (R1_4_3_015 RC1)

2005-09-27 skold <chourf@deware.com>
	* Modified Files
	  common/struct_def.h common/sys_def.h
	  ircd/chkconf.c ircd/ircd.c ircd/s_conf.c ircd/s_conf_ext.h
	  ircd/s_serv.c ircd/s_user.c support/Makefile.in
	* Added Files
	  ircd/config_read.c, fileio.c, fileio.h:
		идиотское #include заменено другим от ratbox, взято из 2.11
	* ircd/s_user.c/m_umode(): 'S' не заменялось на 's' в ограничении
		пользовательского режима ssl

2005-09-23 skold <chourf@deware.com>
	* common/struct_def.h ircd/ircd.c ircd/s_conf.c ircd/s_serv.c
	  ircd/s_user.c rusnet-doc/ChangeLog.ru rusnet-doc/RELEASE_NOTES:
		несколько мелких исправлений для ограниченного имени
		пользователя (IsRLined удалена) и m_trace() (макро
		SendWallops пропускалось)

2005-09-21 erra <erra@rinet.ru>
	* common/send.c: sendto_log передвинута вперёд, чтобы избежать
			ошибок при сборке; если для лога уже есть открытый
			дескриптор, он используется вновь, нескольких
			fd на один лог больше нет.

1.4.3_014 ()
2005-09-17 skold <chourf@deware.com>
	* common/send.c: исправлено переполнение буфера в sendto_log (slcio)
	* ircd/s_conf.c: исправление do_restrict() для ограниченных ('%')
			пользователей
	* rusnet/rusnet_cmds.c: пользовательский режим +h убран из кода
		
2005-09-17 skold <chourf@deware.com>
	* ircd/s_serv.c, channel.c, send.c, common/struct_def.h, parse.c, 
		support/config.h.dist: команда wallops теперь может посылать
			сообщения в канал &WALLOPS, если задана переменная
			WALLOPS_TO_CHANNEL.
			Теперь эта команда доступна операторам.
			
2005-09-17 slcio <silence@irc.run.net>
	* common/parse.c, struct_def.h: #define LOCOP_TLINE/OPER_TLINE for
			opers/locops temporary lines management.

2005-09-12 erra <erra@rinet.ru>
	* ircd/s_conf.c, ircd/s_conf_ext.h: игнорировать строку B: и
			продолжать искать приемлемую строку I:, когда
			записанного в B: сервера нет в сети.

1.4.3_013 (R1_4_3_013)
2005-09-07 skold <chourf@deware.com>
	* ircd/list.c/free_server(),make_server(): добавлено дополнительное
			обнаружение повторных вызовов free()
	* ircd/ircd.c, support/config.h.dist: Параметр DISABLE_DOUBLE_CONNECTS
			импортирован из 2.11, чтобы избавиться от проблем с
			двумя автосоединениями друг к другу, переработана
			функция try_connections() (переделана из 2.11)

1.4.3_012 (R1_4_3_012)
2005-09-04 skold <chourf@deware.com>
	* ircd/s_user.c: исправлена ошибка сегментирования при шифровании
			хоста клиента. Хосты с цифрами в TLD не должны
			отсылаться родительским серверам. Вместо этого им
			отправляется KILL.

2005-09-03 slcio <silence@irc.run.net>
	* support/Makefile.in: исправлена ошибка с codepagedir

2005-09-01 skold <chourf@deware.com>
	* common/send.c, common/send_ext.h, ircd/list.c, s_misc.c, s_user.c,
		s_user_ext.h, support/config.h.dist, common/struct_def.h:
			исправлено переполнение буфера в sendto_log(),
			sendto_flog() переделано, отказ от последнего патча
			для логгинга в (fd merge). Обновите до этой версии,
			если используете p11 или p10.

1.4.3_011 ()

2005-08-28 slcio <silence@irc.run.net>
	* common/send.c: некорректная отправка vhost в vsendpreprep()

2005-08-27 slcio <silence@irc.run.net>
	* many files: избавились от __P()

2005-08-26 skold <chourf@deware.com>
	* support/configure.in, configure: исправлена ошибка с -lcrypto, при
			отсутствии библиотеки не блокирует обнаружение
	* common/support.c, support_ext.h: MyFree(char *) заменили на
			MyFree(void *), gcc-4.x не будет компилировать
				{(char *)(x) = NULL}.


2005-08-24 erra <erra@rinet.ru>
	* common/struct_def.h, ircd/channel.c, ircd/channel_ext.h,
		ircd/s_user.c: введен новый канальный режим: +h (полуоператор)
	* common/struct_def.h, ircd/s_serv.c: выделена ветка SV_2_11 для
			дальнейшего использования: планируется изменить
			версию протокола на 0211 при изменении команды FORCE
			на SVSNICK и таким образом отделить версии.

2005-08-23 erra <erra@rinet.ru>
	* iauth/mod_dnsbl.c: добавлено использование белых списков
			на основе DNS

1.4.3_010 (R1_4_3_010)
2005-08-23 lost <LoSt@lucky.net>
	* ircd/s_serv.s: поправка к описаниям серверов: чрезмерно длинное
			описание обрезалось, если не помещалось в буфер.
	
2005-08-21 slcio <silence@irc.run.net>
	* common/support.c, iauth/a_io.c, ircd/s_bsd.c, iserv/i_io.c,
		support/Makefile.in, support/config.h.dist, ssl_cert:
			операционные системы sparc-solaris-2.[7-9] теперь
			полностью поддерживаются.
	    
2005-08-20 skold <chourf@deware.com>, slcio <silence@irc.run.net>
	* support/configure.in, Makefile.in, config.guess, config.h.dist,
		config.sub, configure, setup.h.in, common/os.h: полностью
			переписан configure.in обнаружение openssl
			переработано, устаревшие макросы удалены
	* irc/*: поддержка клиента удалена
	* различные файлы: все SPRINTF, mystr[n]cmp. strcase[n]cmp указывают
			на my[n]cmp

2005-08-18 skold <chourf@deware.com>
	* common/struct_def.h, ircd/ircd.c, ircd_ext.h, s_bsd.c, s_bsd_ext.h,
		s_conf.c, s_user.c, ssl.c, ssl.h: большая зачистка SSL,
			устранение ошибок. Ошибки в rehash при включении и
			выключении SSL для listener'а. Команда REHASH SSL
			теперь только повторно инициализирует библиотеку SSL
			и сертификаты, а REHASH P перезагружает все listener'ы
			из конфигурации. Реализовано изменение P->p и p->P
			на ходу без переоткрытия порта.

2005-08-17 slcio <silence@irc.run.net>
	* ircd/channel.c: исправления в использовании IsRusnetServices().
	* ircd/ssl.c: исправлено переполнение буфера в disable_ssl().
	
2005-08-15 lost <LoSt@lucky.net>
	* common/send.c, send_ext.h, struct_def.h, iauth/a_log_def.h,
		ircd/ircd.c, ircd_signal.c, s_bsd.c, s_conf.c, s_misc.c,
		s_serv.c, s_user.c, iserv/i_log_def.h, support/config.h.dist:
			новая схема журналирования, настраиваемая из
			ircd.conf, а не config.h.
			Подробности в RUSNET_DOC.

2005-08-15 skold <chourf@deware.com>
	* ircd/ircd.c, s_bsd.c: исправление для daemonize() из 2.11: закрытие
			неверных дескрипторов приводило к нестабильной работе
			сервера после рестарта; удалено древнее BOOT_CONSOLE.

2005-08-14 lost <LoSt@lucky.net>
	* common/common_def.h: макро isvalid() обрезало некоторые украинские
			символы (исправлено).

1.4.3_009 (R1_4_3_009)

2005-08-12 skold <chourf@deware.com>
	* ircd/ircd.c: вызов daemonize() поставлен до вызова initconf(). 
			Некоторые активные дескрипторы, то есть слушающие
			сокеты, могут получать номера наподобие stderr (2)
			и потому быть закрыты вызовом fork().

2005-08-10 slcio <silence@irc.run.net>
	* ircd/s_user.c, common/parse.c, struct_def.h: макол IsRusnetServices()
	* ircd/channel.c, s_user.c: разрешены удалённые cmode, kick, topic,
			invite, notice для IsRusnetServices()
	* ircd/channel.c: разрешены удалённые NAMES для IsOper()
	* ircd/s_serv.c: показывает порты SSL в report_listeners()
		
2005-08-01 skold <chourf@deware.com>
	* ircd/s_serv.c: поддержка маркеров времени в строках T, а не только
			часов в handle_tline().
			Исправление для некорректных бродкастов серверных
			шаблонов в sendto_slaves()
			Исправление для двоеточий в описаниях строк T, замена
			их на запятые.
			
2005-08-01 skold <chourf@deware.com>
	* common/numeric_def.h, ircd/s_bsd.c, s_err.c, support/config.h.dist: 
			RPL_HELLO для незарегистрированных клиентов, массив
			локальных ответов расширен в размере до 20
	* common/struct_def.h, irc/c_numeric.c, ircd/ircd.c, s_err.c, s_serv.c:
			RPL_TRACELOG удалено
	* common/parse.c, struct_def.h, s_bsd.c, s_bsd_ext.h, s_misc.c,
		s_serv.c: вызовы m_reconnect(), close_held(), hold_server()
			удалены
	* common/send.c, ircd/s_bsd.c, s_misc.c, s_user.c:
			исправлены #ifdef UNIXPORT
	* common/struct_def.h, ircd/ircd.c, s_conf.c, s_misc.c, s_misc_ext.h, 
		s_serv.c, support/config.h.dist: 
			теперь CACHED_MOTD включено по умолчанию, read_motd()
			теперь ведёт себя корректно, когда в конце файла
			нет '\n'.
	* common/struct_def.h, ircd/ircd.c, s_bsd.c, s_conf.c: isListening()
			переименовано в isListener()
	* common/struct_def.h, ircd/ircd.c, ircd_ext.h, s_bsd.c, s_bsd_ext.h, 
		s_debug.c, s_misc.c, s_serv.c, support/config.h.dist: 
			параметр DELAY_CLOSE для задержки закрытия соединения
			против быстро возвращающихся клиентов импортирован
			из 2.11
	* common/struct_def.h, ircd/s_bsd.c: коды EXITC_* импортированы из
			2.11 на будущее
	* common/support.c, support_ext.h, ircd/chkconf.c, s_conf.c,
		iserv/iserv.c: dgets() переписан заново (из 2.11)
	* ircd/channel.c: слишком много каналов в команде PART могли быть
			обрезаны, что приводило к рассинхронизации сети.
	* ircd/ircd.c, ircd_ext.h, s_bsd.c: listener'ы удалены из local[] в 
			связанный список ListenerLL
	* ircd/ircd.c, s_bsd.c: iauth теперь работает под cygwin, daemonize(), 
			open_debugfile() импортированы из 2.11
	* ircd/ircd_signal.c: SIGUSR1 убран из обработчиков iauth
	* ircd/list.c, ircd.c, s_bsd.c, s_conf.c: переменная статуса
			serverbooting добавлена для проверки, есть ли у нас
			всё ещё терминал, на который можно посылать сообщения.
	* ircd/s_bsd.c, s_bsd_ext.h: флаг dolisten добавлен в inetport()
			для использования в будущем с флагами строк P:
	* ircd/s_bsd.c, s_bsd_ext.h: вызовы open_listener(), reopen_listeners()
			добавлены для работы со слушающими сокетами.
	* ircd/s_bsd.c: отправка данных в iauth оптимизирована (импортировано
			из 2.11)
	* ircd/s_bsd.c, s_bsd_ext.h: close_client_fd() для закрытия
			действующего клиентского сокета,
			add_connection_refuse() (импортировано из 2.11)
	* ircd/s_bsd.c: close_connection() для следующего соединения может
			быть 0, - это заставляло автосоединение выключаться.
	* ircd/s_bsd.c: задан параметр LISTENER_MAXACCEPT вместо жёстко
			зашитого 10 для определения, как много соединений
			может быть принято одновременно.
	* ircd/s_bsd.c, support/config.h.dist: read_message(): LISTENER_DELAY
			для приёма новых соединений (импорт из 2.11)
	* ircd/s_serv.c: report_listeners() для просмотра настроенных слушающих
			портов и статистики по STATS P; теперь STATS p и
			STATS P отличаются.
	
1.4.3_008 (R1_4_3_008)
2005-06-15 cj <cj@deware.com>
	* ircd/s_conf.c: исправлено некорректное сегментирование в check_tlines()

2005-05-31 cj <cj@deware.com>
	* iserv/iserv.c: изменена настройка сигналов. SIGUSR2 теперь
		игнорируется, SIGHUP переоткрывает файлы журналов/syslog,
		SIGTERM/SIGINT приводят к выходу модуля.

2005-05-30 cj <cj@deware.com>
	* ircd/s_user.c, rusnet/rusnet_cmds.c: внутренний флаг для m_nick()
		должен прекратить проблемы с обработкой коллизий для
		пользовательского режима +r/+R.

2005-05-30 witch <witch@amber.ff.phys.spbu.ru>
	* ircd/s_user.c: m_umode() switch/case переписан в if/else для
		корректности последующих добавлений

2005-05-25 skold <chourf@deware.com>
	* common/match.c, struct_def.h, common_def.h: новый массив символов
		импортирован из кода hybrid, старый массив и макросы будут
		исключены из работы в будущих сборках.
	* ircd/s_serv.c: nonwilds() импортирован из кода hybrid для контроля
		самоубийственных действий операторов.

2005-05-15 skold <chourf@deware.com>
	* ircd/s_conf.c, s_conf_ext.h: find_kill(), find_rline() заменены на
		check_tlines()
	* ircd/s_serv.c: handle_tline(), sendto_slaves(), send_tline_one(),
		rehash_tline() добавлены для работы с временными строками
		конфигурации.

2005-05-15 silencio <silence@irc.run.net>
	* ircd/s_serv.c: match_tline() для проверки временных строк на сервере
	* ircd/s_serv.c: вывод STATS r перемещён в STATS t output. STATS r
		теперь аналог STATS R
			
2005-05-13 silencio <silence@irc.run.net>
	* iserv/iserv.c, iserv.h: поправки к работе с буферизированными файлами
	* iserv/iserv.c: поправки к обратной совместимости чтения масок хостов
	* ircd/s_user.c, s_serv.c: из кода убраны сравнения с SERVICES_HOST
	* ircd/ircd.c, s_conf.c: флаг USE_SERVICES убран из кода, связанного с
			работой сервисов RusNet
	* ircd/*: исключения из K: (строки E:) выведены за пределы флага
			RUSNET_IRCD
	* ircd/ircd.c: checkpings() использует check_tlines() вместо ложного
			find_kill()
	* ircd/s_conf.c, common/struct_def.h: добавлен флаг E для перечитки
			по REHASH исключений из киллов (строк E:)
	
2005-05-12 skold <chourf@deware.com>
	* iserv/i_log.c, i_log_ext.h, iserv.h: реализовано надёжное
			журналирование
	* iserv/iserv.c: сверка строк убрана из кода (будет внутри ircd)
	* ircd/s_serv.c, s_serv_ext.h: m_kline() - полностью переписана и
			перемещена из иерархии rusnet/;
			новые обработчики команд для m_rline(), m_eline(),
			m_ukline(), m_urline(), m_uneline()
	* ircd/s_serv.c, err.c: m_stats() добавлены новые форматы
			RPL_STATSKLINE, RPL_STATSRLINE 
	* ircd/s_conf.c: initconf() теперь заполняет aconf->hold информацией
			о сроке действия временных строк (tline).
	
2005-05-10 skold <chourf@deware.com>
	* ircd/ircd.c, support/config.h.dist: TIMEDKLINES убраны из кода
	* ircd/s_conf.c: find_kill() переработан в связи с новым форматом
			строк K:
	* common/match.c, ircd/s_conf.c: match(), match_ipmask(): пропускают
		начальное '=' при сравнении имён хостов.

2005-05-09 skold <chourf@deware.com>
	* iserv/*, support/Mekafile.in, config.h.dist, ircd/channel.c,
		s_bsd.c, s_conf.c, s_iserv.c, s_iserv_ext.h, common/send.c,
		struct_def.h: модуль iserv как замена tkserv

2005-05-07 silencio <silence@irc.run.net>
	* ircd/ircd.c: main(): iserv, запускаемый ircd
	
1.4.3_007 (R1_4_3_007 tag)

2005-04-27 skold <chourf@deware.com>
	* ircd/s_conf.c: rehash() добавлена перечитка resolv.conf для
			/rehash dns

2005-04-22 skold <chourf@deware.com>
	* common/struct_def.h, ircd/ircd.c, s_conf.c: FLAGS_RLINE и имя
			пользователя меняются в соответствии со строками R:
	* ircd/s_conf.c: attach_conf() теперь u@h лимиты для строк R:
			не берутся из I:
	
1.4.3_006 (R1_4_3_006 tag)
2005-04-18 skold <chourf@deware.com>
	* contrib/tkserv/tkserv.c: squery_line() добавлена работа со строками
			R:, поправдены исключения
	* rusnet/rusnet_cmds.c, ircd/s_serv.c, common/struct_def.h: команда
			протокола RLINE
	* common/patchlevel.h, support.c, ircd/s_err.c, ircd.c: make_version()
			адаптирована для RusNet
	* ircd/ircd.c: вызов check_pings() для строк R: по команде REHASH
	* ircd/channel.c: несколько исправлений ошибок для режима +R

1.4.3_005 (R1_4_3_005 tag)

2005-03-31 skold <chourf@deware.com>
	* ircd/s_conf.c, s_serv.c: m_rehash(), rehash() полностью переписаны,
			initconf() поправлен на предмет разбора дополнительного
			параметра, задающего, что перечитывать
	* ircd/s_conf.c: initconf() рекурсия строк #include переписана
			(содержала ошибки)
	* ircd/s_conf.c, ircd.c, chkconf.c, common/struct_def.h: старые строки
			R: и флаг CONF_RESTRICTED убраны из кода

2005-03-26 skold <chourf@deware.com>
	* common/numeric_def.h, struct_def.h, ircd/chkconf.c, s_conf.c,
		s_conf_ext.h, s_err.c, s_user.c: реализованы строки RusNet R:

2005-03-29 silencio <silence@irc.run.net>
	* support/Makefile.in, config.h.dist, ircd.rmotd: сокращённый motd

1.4.3_003 (-)

2005-03-17 cj <cj@deware.com>
	* support/configure.in, configure: правки для сборки под SunOS и Linux

2005-03-19 skold <chourf@deware.com>
	* support/config.h.dist, ircd/s_user.c, channel.c, common/struct_def.h:
		появился пользовательский режим +R для RusNet

1.4.3_002 (R1_4_3_002 tag)

2003-11-13 cj <cj@deware.com>
	* support/configure.in, acconfig.h, configure, setup.h.in: правки для
			обнаружения OpenSSL
	* support/configure.in, config.h.dist, Makefile.in: обнаружение
			устройства случайных данных и сокета EGD

2003-10-30 skold <chourf@deware.com>
	* common/numeric_def.h, os.h, send.c, send_ext.h, struct_def.h,
		irc/c_defines.h c_externs.h, ircd/chkconf.c, ircd.c,
		ircd_ext.h, s_bsd.c, s_conf.c, s_debug.c, s_err.c, s_externs.h,
		s_serv.c, s_user.c: добавлена подержка клиента SSL
	* support/config.h.dist: исправления/улучшения, не связанные с SSL
	* support/configure.in: необходимые изменения для SSL

1.4.2R (R1_4_2 tag)

2004-12-24 silencio <silence@irc.run.net>
	* ircd/s_user.c/m_nick(): исправление для (NICK 2) направления KILL
			по коллизии с сервисами

2004-12-07 skold <chourf@deware.com>
	* common/parse.c/parse(): код возврата -1 в игнорировании префикса
			изменён на 1 (вызывал инвалидацию fd)
			Обновление (хабов) ОБЯЗАТЕЛЬНО.

2004-12-06 skold <chourf@deware.com>
	* ircd/channel.c/clean_channelname(): буква ё и кодировка koi8-u


1.4.2 (deferred)

2004-10-06 skold <chourf@deware.com>
	* ircd/channel.c: каналы не задерживаются после последнего удалённого
			KILL на оператора канала
	* ircd/channel.c/clean_channelname(): улучшен разбор названий каналов

2004-09-21 erra <erra@rinet.ru>
	* contrib/tkserv/tkserv.c: два небольших исправления к переполнениям
			буферов в tkserv

2004-09-14 erra <erra@rinet.ru>
	* iauth/mod_dnsbl.c: исправлена обработка кэша и финальная точка к
			имени хоста

(1.4.2pre_20)

2004-08-19 skold <chourf@deware.com>
	* channel.c/match_mode_id(): устранена утечка памяти, внесенная
			2003-09-24

2004-08-09 skold <chourf@deware.com>
	* ircd/channel.c: поправлена ошибка переполнения буфера в pre_18

2004-07-06 erra <erra@rinet.ru>
	* rusnet/rusnet_cmds.c: причина передаётся из KLINE в tkserv

2004-05-16 skold <chourf@deware.com>
	* ircd/s_user.c: исправлено переполнение буфера команды
		"KICK #chan me, someone" (ошибка NJOIN),
		исправлено отслеживание kill'а на клиентских серверах,
		получивших KILL в результате коллизии

2004-05-16 silencio <silence@irc.run.net>
	* ircd/s_user.c: исправлена некорректная обработка сервисных коллизий

2004-05-02:
* убран код сумеречного режима  -skold
* временный код для резолвера во FreeBSD jail  -skold
* исправлено переполнение буфера в функции add_local_domain()  -skold
* добавлена возможность заходить на тихие (+q) каналы вне зависимости
  от MAXCHANNELSPERUSER  -skold

2004-03-31:
* отработка команды STATS для локальных операторов на своём сервере  -skold
* исправлена маскировка user@host для команды STATS o  -skold
* исправлено переполнение буфера в umode +x для одноуровневых доменов (без
  точки)  -erra

2004-02-27:
* исправлена инициализация таблиц to<low|up>ertab[] в KOI8  -skold
* небольшое исправление к заданию параметра client_flood  -skold

2004-02-24:
* исправлено удаление K/E строк посредством tkserv  -erra

2004-02-14:
* К логам канала &KILLS добавлены ident@host  -skold
* исправлены умолчания в config.h.dist  -skold

2004-02-12:
* небольшое исправление к спискам коллизий, чтобы усложнить намеренную
  коллизию  -skold

2004-02-05:
* изменён алгоритм VHOST для лучшей маскировки  -LoSt

2004-02-04:
* VHOST_MODE_MATCH теперь включён всегда  -skold

2004-02-03:
* наконец исправлена работа со строками K/E в tkserv  -erra

2004-01-16:
* исправлена работа с кэшированными записями DNSBL  -erra

2004-01-11:
* исправления к expire_collision_map  -skold

2004-01-09:
* исправлена ошибка сегментирования в add_history(pre_8) -skold
* убрано ретранслирование сообщения об ошибке в m_umode -skold
* добавлен KILL зависших коллизий в expire_collision_map  -erra

2004-01-07:
* добавлен префикс ником в RPL_WHOISHOST  -erra
* введен RPL_CHARSET вместо жёстко заданного ответа (в whois)  -erra
* в механизм устаревания коллизий добавлен вывод списка коллизий  -skold
* изменена работа с переменной invincible и параметром SERVICES_SERV:
  добавлен KILL для enforcer'а  -skold
* введен в работу неиспользуемый ранее aCollmap->tstamp  -skold
* улучшена совместимость в common/parse.c -skold

2004-01-06:
* добавлена команда EXEMPT в tkserv (теперь v1.4.0)  -erra
* добавлен параметр компиляции TKSERV_VERBOSE  -erra
* добавлен синтаксис "TKLINE E ..." для взаимодействия с сервисами  -erra
* параметр TK_MAXTIME перенесён из config.h в tkconf.h  -erra
* жёстко записанные значения в tkserv заменены параметром TK_MAXTIME  -erra

2004-01-05:
* добавлено исключение для коллизий ников, приходящих от сервисов -
  их не меняем  -erra
* части кода отмечены #ifdef USE_SERVICES  -erra
* вновь написано ограничение срока действий списков коллизий  -erra

2003-12-30:
* пользовательский режим +h убран  -erra
* существенно изменена обработка коллизий  -erra, skold
* m_restart - поправлен для режима +x  -skold

2003-12-12:
* поправлена некорректная команда :old NICK :new для коллизии ника  -skold
* поправлено обрезание цифр в конце ника  -skold
* небольшая поправка к m_quit  -skold
* канал &LOCAL теперь якобы только по приглашениям  -skold
* карты коллизий должны быть сильно переписаны в связи с ошибкой
  алгоритма  -skold

2003-12-09:
* добавлен параметр MODES_RESTRICTED: запрещает /mode #channel +e/+I для
  тех, кто не на канале (и не оператор IRC)  -erra
* также запрещён /mode #channel +I для всех, кроме операторов канала
  (или  IRC)  -erra
* добавлен лог для канала &LOCAL (отдельный от userlog)  -erra
* реструктурированы определения логов в config.h для лучшей читаемости  -erra

2003-11-30:
* исправлен поиск host/ip = ... and host/ip = !... в iauth.conf  -erra
* добавлен "!" для строк исключений в вывод /stats A  -kmale
* исправлена опечатка в ircdwatch.c  -erra
* поправлены таблицы tolower/toupper в common/match.c.
  TODO: обойтись без них вовсе  -erra

2003-11-28:
* убран чёрный список EasyNet из support/iauth.conf  -erra
* убран статистический отчёт при запросе удалённого сервера в /lusers,
  поскольку цифры неверны  -erra

2003-11-16:
* малая поправка к get_client_xname (неверный формат)  -skold
* дополнительная отладка к valloc и спискам коллизий  -skold

2003-11-14:
* переделан get_client_xname для m_trace  -erra
* закомментированы все конструкции #undef MACRO value, чтобы убрать
  предупреждения gcc3.3  -erra

2003-11-08:
* поправлен выход за границы массивов в m_service и m_nick  -skold
* поправлен запрет на ник anonymous, который по сути не работал  -skold

2003-11-07:
* поправлена ошибка сегментирования, внесенная поправкой к opnotices  -skold
* поправлен m_trace, чтобы не показывал настоящий хост пользователей
  в режиме +x  -skold

2003-11-04:
* исправлена передача opnotice/opmsg между серверами  -kmale
* поправлен нотис операторам о проходе через бан  -kmale
* добавлен нотис оператору IRC при проходе через бан  -erra
* восстановлены настоящие хосты для сервисов (S:)  -erra

2003-10-29:
* отчёт о глобальных пользователях  -erra
* поправлено неверное распространение команд, внесенное вчера  -erra
* исправлен RPL_BOUNCE  -erra

2003-10-28:	[pre-release]
* добавлены строки E: (исключения из K:)  -erra
* поправлена ошибка логики в s_bsd.c (клиентский флуд)  -erra
* client_flood и localpref документированы в example.conf  -erra
* строки E: документированы в example.conf  -erra
* исправлена ошибка сегментирования вследствие вызова lock_nick после
  exit_one_client  -erra
* добавлен синтаксис "KLINE *" или "KLINE *.domain" для m_kline  -erra

2003-10-27:
* добавлена поддержка RPL_ISUPPORT, изменён код ответа RPL_BOUNCE  -erra
* небольшая модернизация производительности в channel.c  -erra
* добавлен параметр client_flood (клиентский флуд) в строки I: (пишется
  в aconf->localpref)  -erra
* добавлен код EXTRA_STATISTICS (из IRCNet ircd)  -erra

2003-10-24:
* добавлена инициализация для localpref (в 0)  -erra
* слегка улучшен генератор случайных чисел  -erra
* отрезаны лишние цифры из нового ника при коллизии  -erra
* исключена функция strcpyzt (заменена на strncpyzt)  -erra
* новая возможность: отсылка на тот же порт, на который пришёл клиент
  (B:)  -erra

2003-10-20:
* внесены поправки к mod_dnsbl (например, кэш результатов) от Beeth
* исправлена обработка случая "list = somehost.bl.org" (только один хост)
  для mod_dnsbl  -erra

2003-10-18:
* поправлено отображение настоящего хоста в логах и системных каналах  -erra

2003-10-13:	[Brain Damage]
* спешная адаптация пакета irc2.10.3p5 package, исправление к переполнению
  буфера в m_join buffer  -erra
* исправлена зависшая опечатка в config.h.dist (комментарий к MAXLOCKS)  -erra

2003-10-12:
* добавлена защита от переполнения в алгоритм маскировки хоста  -erra

2003-10-10:
* добавлен параметр localpref в вывод команды /stats c  -erra

2003-10-09:
* исправлено чтение строки "ip = !1.2.3.4" из iauth.conf  -erra
* исправлено чтение строки "ip = ..." из iauth.conf  -erra
* исправлен отчёт "IP = ..." при вызове iauth -c iauth.conf  -erra

2003-10-07:
* устранено зацикливание в списка коллизий (при коллизии смены ника)  -erra

2003-10-06:
* исправлен механизм рестарта сервера
  (переписана обработка сигналов: ircd/ircd_signal.c):
  - SIGCHLD обрабатывался некорректно
  - s_restart() использовал устаревшую обработку сигналов
  - restart() устарел и посему удалён
  - server_reboot() не отслеживал потомков ircd - исправлен
  - #ifdef PROFIL и s_monitor устарели - удалены  -skold
* убраны my[n]cmp из common/match.c, все вызовы заменены на
  str[n]casecmp  -erra

2003-09-30:
* исправлено скрытие хостов для строк O: (не работало)  -erra
* добавлен цифровой код для /stats f  -erra

2003-09-29:	[Equinox+]
* исправлена обработка синтаксиса "nick!"  -erra
* исправлена ранее испорченная обработка юникс сокетов в common/send.c  -erra
* ограничена команда STATS для не операторов до C/H/O/U, хосты из строк
  O: спрятаны для не операторов  -erra
* два новых параметра в config.h - VHOST_MODE_MATCH (по умолчанию выключен) и
  STATS_RESTRICTED (включен по умолчанию)  -erra
* изменена буква строк X: на F:  -erra
* добавлена команда /stats f - показать строки F:  -erra
* нотис "стал оператором IRC" исправлен, чтобы сообщать настоящий хост  -skold

2003-09-28:
* исправлено выделение памяти под клиенты  -skold
* добавлен нотис операторам IRC о тех, кто пытается получить +o  -skold
* ещё одна маленькая поправка к алгоритму шифрованных хостов  -erra
* команда STATS запрещена всем, кроме операторов сети  -skold
* добавлен синтаксис "nick!" для получения "nick +x" в момент
  соединения  -skold, -erra

2003-09-25:
* изменён алгоритм шифрования хостов  -erra

2003-09-25:
* добавлен mod_webproxy.c (взят из IRCNet ircd)  -erra
* изменён mod_webproxy в соответствии с общим стилем  -erra
* doc/iauth.conf.5 и support/iauth.conf обновлены соответственно  -erra
* добавлен подробный ответ от модуля rfc931  -erra
* добавлен параметр NO_DIRECT_VHOST в support/config.h.dist и m_umode  -erra
* дополнительные зависимости в support/Makefile.in

2003-09-24:
* изменения в алгоритм шифрования хостов, шифрование усилено  -erra
* исправлен ответ 002  -erra
* исправлена ранее поломанная ошибка "забанен на канале"  -erra
* исправлена функция match_modeid для шифрованных хостов  -erra
* убран статический буфер из make_nick_user_host  -erra
* нотисы входа сквозь бан по приглашению и операторы заменены на
  операторские (opnotice)  -erra
* добавлен настоящий хост в данные whowas  -erra

2003-09-23:
* закончена разработка виртуальных (шифрованных) хостов для пользовательского
  режима +x  -erra
* несколько малых правок в разных местах кода  -erra
* добавлен параметр NO_DEFAULT_VHOST в config.h  -erra

2003-09-22:	[Equinox]
* исправлена обработка команды RMODE до полностью рабочего состояния  -erra
* директива "connect thru" перемещена в ircd.conf (строки x:)  -erra
* исправлен chkconf.c для распознания строк X:  -erra
* начата работа над маскированием хостов пользователей (только операторы
  IRC могут видеть настоящий хост по /whois), оставлено для TODO  -erra
* заменён код ответа "изменена кодовая страница"  -erra
* заменены последние два символа в таблице b64, чтобы лучше соответствовать
  стандартам DNS  -erra
* проверка идента на корректность перемещена в register_user  -erra

2003-09-19:
* ещё более плавная работа проверок в can_join()  -erra
* изменено поведение в m_invite() - запрещено приглашение на
  несушествующий канал  -kmale, -erra
* оптимизированы функции списка коллизий по производительности, добавлен
  отладочный вывод  -skold

2003-09-18:
* исправлены "сумеречные" нотисы для операторов сети, входящих на канал,
  в случае если для них есть исключение или приглашение  -kmale, -erra

2003-09-14:
* малая поправка к s_bsd.c (код возврата)  -Umka
* причина отказа авторизации соединения перемещена в cptr->name,
  ибо в остальных местах она перезаписывается другими данными  -erra
* сделан отдельный флаг для коллизии  -erra, -Umka

2003-09-13:
* введены читабельные отказы авторизации от iauth  -erra
* небольшая поправка к выводу mod_dnsbl.c в /stats a и iauth -c  -erra

2003-09-11:
* много поправок к mod_dnsbl.c, добавлен параметр "paranoid"  -skold, -erra
* обновлён doc/iauth.conf.5 по поводу параметра "paranoid"  -erra

2003-09-10:
* устранена ошибка сегментирования в mod_dnsbl.c  -erra
* исправлен возврат без заданного значения в shrink_collmap()  -erra
* малая поправка к a_conf.c (iauth)  -erra
* устранена небольшая течь памяти в m_nick (s_user.c, обработка коллизии
  :old NICK :new)  -erra

2003-09-09:	[rc7]
* исправлено условие проверки списка коллизий в common/parse.c  -erra
* переделан вызов m_nick из обработки коллизий  -erra

2003-09-08:
* убрано сообщение о коллизии в обработке коллизии смены ника :old NICK :new
  как излишнее  -erra

2003-09-07:
* ещё немного поправок к спискам коллизий  -erra
* устранена ошибка сегментирования в del_from_collision_map  -erra
* начат ответник (FAQ)  -erra

2003-09-05:
* тщательно переработаны списки коллизий, переписано выделение памяти  -erra
* удалены codepages-rusnet1.3/ как устаревшие  -erra

2003-09-02:
* изменены ircd/s_err.c и ircd/s_users.c на предмет возврата времени
  входа пользователя в выводе whois  -erra
* введены списки коллизий  -erra

2003-09-01:
* введена модифицированная функция crc32+b64 (ircd/hash.c), добавлено поле
  контрольной суммы в структуру aServer для обработки коллизий ников  -erra

2003-08-29:
* протестирована и исправлена обработка коллизии "NICK new", введена
  (но не протестирована) обработка коллизии ":old NICK :new"  -erra
  Результаты печальны, нужен другой алгоритм.

2003-08-28:
* изменено условие в m_force чтобы не посылать KILL в неверном
  направлении  -erra
* косметические правки rusnet/rusnet_cmds.c  -erra
* исправлена ошибка сегментирования и отладочный вывод в tkserv  -skold
* изменена функция m_nick с целью форсирования корректных ников,
  недоработано  -erra

2003-08-27:
* малые поправки к ircdwatch  -skold
* изменён список DNSBL по умолчанию в support/iauth.conf  -erra
* добавлена ещё одна замена KILL на FORCE в m_nick  -erra

2003-08-26:
* добавлен вызов m_kill в m_force для старых серверов, которые
  не поддерживают эту команду  -erra

2003-08-25:
* исправлен support/iauth.conf: изменено слово 'host' на 'list' в параметрах
  dnsbl  -erra

2003-08-05:
* исправлен механизм удержания ников - в config.h внесены новые
  параметры, связанные с удержанием ников  -kmale
* удержание ников исследовано и слегка скорректировано  -erra

2003-08-04:
* обновлён iauth/a_conf.c, чтобы позволить ip = 1.2.3.4 также как
  ip = 1.2.3.4/32 в iauth.conf  -erra

2003-08-01:
* дописан mod_dnsbl.c для iauth, чтобы позволить обращение к нескольким
  чёрным спискам  -erra
* iauth.conf.5 дополнен информацией о dnsbl  -erra

2003-07-30:
* добавлен пример 'ip = ...' в справочную страницу iauth.conf.5  -erra

2003-07-29:
* исправлено вырезание списка каналов для ChanServ  -erra

2003-07-18:
* добавлен логгинг канала &AUTH (в connlog)  -erra
* исправлен двойной логгинг (в файлы и syslog)  -erra

2003-07-17:
* исправлены непроверенные пути логов и механизм записи логов вообще  -erra
* исправлен mod_dnsbl  -erra
* добавлено ведение лога канала &LOCAL  -erra

2003-07-16:	[rc6]
* включено ведение логов для каналов &ERRORS и &NOTICES в файл(ы)
  (настраиваемо), все имена файлов для логов перемещены из Makefile
  в config.h  -erra
* начат модуль dnsbl для iauth  -erra

2003-07-15:
* изменена обработка команд /*serv, чтобы они посылали PRIVMSG service@server
  вместо просто PRIVMSG service  -erra

2003-07-11:
* исправлен support/Makefile.in для сборки chkconf  -erra

2003-07-09:
* добавлена проверка идента на корректность в m_user  -LoSt
* добавлены методы m_*serv для команд /nickserv и т.д.  -erra
* исправленные версии openconf() и dgets() мигрированы в chkconf.c  -erra

2003-07-03:
* dgets() переделан (common/support.c), теперь #include корректно
  работает  -erra
* добавлен лог каналов &ERRORS (*.warn) и &NOTICES (*.info) в syslog  -erra

2003-07-01:	[rc5]
* переписана обработка opnotices/opmsg: выходит, что исходный ircd использует
  chptr->clist->flags в качестве внутреннего счётчика  -erra

2003-06-27:
* исправлена однонаправленная посылка расширений протокола между
  серверами. Есть два места в ircd,  где передаётся стартовая строка
  PASS ... |IRC, до сих пор было сделано только одно из них  -erra
* удалено поле rusnet_proto из структуры Server - вся необходимая информация
  перемещена в поле version  -erra

2003-06-19:
* return 0 добавлен в несколько разных функций в rusnet_cmds  -erra

2003-06-18:
* opnotice/opmsg приведены к рабочему состоянию  -erra

2003-06-16:
* добавлены opnotice/opmsg (/notice @#channel, /msg @#channel)  -erra

2003-06-11:
* исправлена проверка лимитов срока действия в tkserv  -erra

2003-06-06:
* добавлены функцим str[n]casecmp в common/support.c  -erra
* изменено умолчание в iauth.conf на предмет отказа открытым прокси  -erra
* добавлен поиск strcasecmp в скрипт configure и окружение  -erra

2003-06-05:	[rc4]
* исправлена обработка команды KLINE, теперь она работает  -erra
* изменён tkserv, чтобы разрешить добавлять или удалять перманентные
  строки K:  -erra
* убран список каналов для сервисных ников  -baron
* изменён initconf(), чтобы позволить вложенные конфигурации  -erra
* изменены правила в Makefile для tkserv, чтобы он работал с kills.conf вместо
  ircd.conf  -erra

2003-05-06:
* скорректировано поведение для "постоянных" строк K: в rusnet_cmds.c,
  добавлен параметр TK_MAXTIME в config.h для лучшей настройки  -erra

2003-05-05:	[rc3]
* ещё раз переделана обработка строк K: (неправильный вызов приводил к отстрелу
  всех пользователей сервера по команде /rehash, если присутствовала хоть одна
  строка, начинающаяся с K:*:)  -erra
* новая команда протокола: KLINE -baron, -erra
* множественные поправки к rusnet_cmds.c  -erra
* новые параметры в config.h для обработки KLINE  -erra
* команда RCPAGE может быть послана оператором, если задано в config.h  -erra

2003-04-28:
* переработаны и исправлены строки K: для ников. Изменена функция find_kill
  (добавлен новый аргумент) для обеспечения требуемой функциональности  -erra
* удалены ненужные куски кода, связанные с обработкой директивы "interface" в
  rusnet.conf  -erra

2003-04-26:	[rc2]
* новые возможности: приоритет автоконнекта (строки c/C: - параметр localpref
  в структуре Server), поиск ников в строках K:  -erra
* функциональность директивы "connect thru" в rusnet.conf расширена механизмом
  отката (когда указан несуществующий IP, будет использован основной IP
  сервера)  -erra
* удалена директива "interface" из rusnet.conf как избыточная  -erra
* документирован rusnet.conf (см. doc/example.rusnet.conf)  -erra

2003-04-15:
* исправлена работа с кодовыми таблицами  -LoSt
* исправления к m_force, m_rmode, m_rcpage  -erra
* множество обрамлений (#ifdef RUSNET_IRCD) во многих точках кода  -erra

2003-04-02:	[beta3]
* новые команды протокола: RMODE, RCPAGE  -baron
* новый режим канала: +c (цветовыделение запрещено)  -baron
* исправления в codepages.universal  -LoSt
* исправлен ircd/whowas.c  -LoSt
* исправлены common/parse.c и common/send.c  -LoSt
* исправлены configure и configure.in на предмет ложного обнаружения
  Solaris на FreeBSD 5.x  -erra

2002-09-22:	[beta1]
* rusnet/rusnet_cmds.c: изменена обработка команды FORCE с широковещательной
  на персонально маршрутизируемую. Это должно невообразимо уменьшить
  трафик на момент загрузки сервисов  -az

2002-09-21:	начало
* RusNet IRCD теперь основан на ircd 2.10.3p3 (было 2.10.3) с ftp.irc.org,
  практически все куски кода были обрамлены #ifdef RUSNET_IRCD для сохранения
  совместимости и лучшей применимости  -erra


Версия 1.4.1
_____________

PATCHLEVEL 5
------------

Изменения: rusnet/
		rusnet_init.c

	Мусор в описании маршрута, когда описание не введено в директиве
	"connect" файла rusnet.conf после "thru 1.2.3.4", возможно
	переполнение при наличии доступа на запись к rusnet.conf

	срочность: средняя.

PATCHLEVEL 4
------------

Изменения: rusnet/
		rusnet_cmds.c
	 
	решена проблема обвала при обработке команды FORCE в отладочном
	режиме (DEBUGMODE)  -Alex "Umka" Ljashkov

	 common/
		os.c (см.: RUSNET POLL)

	Сделаны изменения для поддержки функции poll() в ОС Linux.

PATCHLEVEL 3
------------

Изменения: rusnet/
		rusnet.h
		rusnet_init.h
		rusnet_misc.c
	 	rusnet_virtual.c
		s_bsd.c (см.: RUSNET CONNECT)

	В rusnet.conf внесены изменения, чтобы позволить перенаправление
	исходящих соединений на отдельно взятый интерфейс (адрес IP)  -Adel


PATCHLEVEL 2
------------

2000-08-28:
* Улучшены WHOIS_NOTICES. Теперь операторы получают нотис вне зависимости
  от того, находятся ли они на запрашиваемом сервере.

2000-08-25:
* Добавлены EXTRA_NOTICES для операторов сети. Теперь можно, видеть кто делает
  WHOIS на тебя. Смотреть JOIN/EXIT/NICKCHANGE на канале &OPER, можно смотреть
  пользователей в режиме +i (невидимок) и пользователей на других каналах,
  не находясь на них. Исправлен Makefile для инсталяции codepages.

2000-08-17:
* Добавлена новая таблица для маков... так называемая MacRoman 
  Big thanks: Alexander Javoronkov

2000-06-27:
* Добавлена поддержка topic set by (код 333)
