// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#if !defined (VALENT_TEST_INSIDE) && !defined (VALENT_TEST_COMPILATION)
# error "Only <libvalent-test.h> can be included directly."
#endif

#include <libvalent-core.h>
#include <libvalent-clipboard.h>

G_BEGIN_DECLS

#define VALENT_TYPE_MOCK_CLIPBOARD_ADAPTER (valent_mock_clipboard_adapter_get_type ())

G_DECLARE_FINAL_TYPE (ValentMockClipboardAdapter, valent_mock_clipboard_adapter, VALENT, MOCK_CLIPBOARD_ADAPTER, ValentClipboardAdapter)

G_END_DECLS

