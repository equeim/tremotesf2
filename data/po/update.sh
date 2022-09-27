#!/bin/sh

# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

sed "/Icon=/d" ../org.equeim.Tremotesf.desktop.in > tmp.desktop.in
xgettext --copyright-holder="Alexey Rochev <equeim@gmail.com>" --package-name="tremotesf" -p ./ tmp.desktop.in ../org.equeim.Tremotesf.appdata.xml.in
rm tmp.desktop.in
sed -i "s/CHARSET/UTF-8/" messages.po
