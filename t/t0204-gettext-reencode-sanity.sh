#!/bin/sh
#
# Copyright (c) 2010 Ævar Arnfjörð Bjarmason
#

test_description="Gettext reencoding of our *.po/*.mo files works"

. ./lib-gettext.sh


test_expect_success GETTEXT_LOCALE 'gettext: Emitting UTF-8 from our UTF-8 *.mo files / Icelandic' '
    printf "TILRAUN: Halló Heimur!" >expect &&
    LANGUAGE=is LC_ALL="$is_IS_locale" gettext "TEST: Hello World!" >actual &&
    test_cmp expect actual
'

test_expect_success GETTEXT_LOCALE 'gettext: Emitting UTF-8 from our UTF-8 *.mo files / Runes' '
    printf "TILRAUN: ᚻᛖ ᚳᚹᚫᚦ ᚦᚫᛏ ᚻᛖ ᛒᚢᛞᛖ ᚩᚾ ᚦᚫᛗ ᛚᚪᚾᛞᛖ ᚾᚩᚱᚦᚹᛖᚪᚱᛞᚢᛗ ᚹᛁᚦ ᚦᚪ ᚹᛖᛥᚫ" >expect &&
    LANGUAGE=is LC_ALL="$is_IS_locale" gettext "TEST: Old English Runes" >actual &&
    test_cmp expect actual
'

test_expect_success GETTEXT_ISO_LOCALE 'gettext: Emitting ISO-8859-1 from our UTF-8 *.mo files / Icelandic' '
    printf "TILRAUN: Halló Heimur!" | iconv -f UTF-8 -t ISO8859-1 >expect &&
    LANGUAGE=is LC_ALL="$is_IS_iso_locale" gettext "TEST: Hello World!" >actual &&
    test_cmp expect actual
'

test_expect_success GETTEXT_ISO_LOCALE 'gettext: Emitting ISO-8859-1 from our UTF-8 *.mo files / Runes' '
    LANGUAGE=is LC_ALL="$is_IS_iso_locale" gettext "TEST: Old English Runes" >runes &&

	if grep "^TEST: Old English Runes$" runes
	then
		say "Your system can not handle this complexity and returns the string as-is"
	else
		# Both Solaris and GNU libintl will return this stream of
		# question marks, so it is s probably portable enough
		printf "TILRAUN: ?? ???? ??? ?? ???? ?? ??? ????? ??????????? ??? ?? ????" >runes-expect &&
		test_cmp runes-expect runes
	fi
'

test_done
