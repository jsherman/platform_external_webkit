/* WebKitAdBlock
 *
 * Copyright (C) 2009 Jonah Sherman <sherman.jonah@gmail.com>
 * Based on AdBlockPlus by Wladimir Palant <trev@adblockplus.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CachedResource.h"
#include "CString.h"
#include "TextEncoding.h"
#include "RegularExpression.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

#include <stdio.h>

namespace WebCore {

#define DOCUMENT_TYPE 9
#define CACHE_SIZE 1009
#define SHORTCUT_SIZE 8
#define FILTER_PATH "/sdcard/block.txt"

class Pattern {
public:
    Pattern() : m_types(0) { }
    Pattern(const RegularExpression& re, int types)
        : m_re(re), m_types(types) { }
    bool matches(const String& target, int type) {
        return ((1<<type) & m_types) && m_re.search(target) >= 0;
    }
    RegularExpression m_re;
private:
    unsigned int m_types;
};

class CacheEntry {
public:
    String target;
    bool block;
};

class PatternMatcher {
public:
    void addPattern(const String& pat);
    bool matches(const String& target, int type);
private:
    Vector<Pattern> m_patterns;
    HashMap<String, Pattern> m_shortcuts;
};

bool ad_block_enabled = false;
static CacheEntry *ab_cache;
static PatternMatcher ab_blackList;
static PatternMatcher ab_whiteList;

void PatternMatcher::addPattern(const String& pat)
{
    int delim = pat.find("$");
    String pattern, optpart;
    if (delim < 0) {
        delim = pat.length();
    }
    optpart = pat.substring(delim+1);
    pattern = pat.left(delim);
    Vector<String> opts;
    optpart.split(",", opts);
    int types = -1;
    bool matchCase = false;
    for (unsigned i = 0; i < opts.size(); i++) {
        const String &opt = opts[i];
        if (opts[i] == "match-case") {
            matchCase = true;
            continue;
        }
        bool invert = false;
        String typeOpt = opt;
        int typeMask = 0;
        if (typeOpt.startsWith("~")) {
            invert = true;
            typeOpt = typeOpt.substring(1);
        }
        if (typeOpt == "image") {
            typeMask = 1<<CachedResource::ImageResource;
        } else if (typeOpt == "stylesheet") {
            typeMask = 1<<CachedResource::CSSStyleSheet;
        } else if (typeOpt == "script") {
            typeMask = 1<<CachedResource::Script;
        } else if (typeOpt == "subdocument") {
            typeMask = 1<<DOCUMENT_TYPE;
        }
        if (invert) {
            types &= ~typeMask;
        } else {
            if (types == -1) {
                types = 0;
            }
            types |= typeMask;
        }
    }
    if (!types) {
        return;
    }
    if (pattern.startsWith("/") && pattern.endsWith("/")) {
        pattern = pattern.substring(1, pattern.length()-2);
        m_patterns.append(Pattern(RegularExpression(pattern), types));
        return;
    }
    int pos = 0;
    RegularExpression nonchar("\\W");
    while ((pos = nonchar.search(pattern, pos)) >= 0) {
        pattern.insert("\\", pos);
        pos += 2;
    }
    replace(pattern, RegularExpression("\\\\\\*"), ".*");
    replace(pattern, RegularExpression("^\\\\\\|"), "^");
    replace(pattern, RegularExpression("\\\\\\|$"), "$");
    String text = pat.left(delim).lower();
    replace(text, RegularExpression("^\\|"), "");
    replace(text, RegularExpression("\\|$"), "");
    for (pos = text.length() - SHORTCUT_SIZE; pos >= 0; pos--) {
        String sub = text.substring(pos, SHORTCUT_SIZE);
        if (sub.find("*") < 0 && !m_shortcuts.contains(sub)) {
            m_shortcuts.set(sub, Pattern(RegularExpression(pattern), types));
            return;
        }
    }
    m_patterns.append(Pattern(RegularExpression(pattern), types));
}

bool PatternMatcher::matches(const String& target, int type)
{
    const TextEncoding &utf8 = UTF8Encoding();
    int i;
    for (i = target.length() - SHORTCUT_SIZE; i >= 0; i--) {
        String sub = target.substring(i, SHORTCUT_SIZE);
        Pattern pattern = m_shortcuts.get(sub);
        if (pattern.matches(target, type)) {
            return true;

        }
    }
    int size = m_patterns.size();
    for (i = 0; i < size; i++) {
        if (m_patterns[i].matches(target, type)) {
            return true;
        }
    }
    return false;
}

// XXX: Figure out how to use existing String hash buried in a nest of templates
static
unsigned ab_hash(const String& str)
{
    unsigned result = 0;
    unsigned size = str.length();
    const UChar *p = str.characters();
    for (unsigned i = 0; i < size; i++) {
        result = 31 * result + (unsigned)p[i];
    }
    return result;
}


static CacheEntry* initialize()
{
    FILE *file = fopen(FILTER_PATH, "r");
    if (!file) {
        file = fopen("/system/etc/block.txt", "r");
    }
    if (file) {
        char buf[512];
        fgets(buf, 512, file);
        while (fgets(buf, 512, file)) {
            String line(buf);
            line.replace("\n","");
            if (line.startsWith("@@")) {
                ab_whiteList.addPattern(line.substring(2));
            } else if (!line.startsWith("!") && line.find("#") < 0) {
                ab_blackList.addPattern(line);
            }
        }
        fclose(file);
    }
    return new CacheEntry[CACHE_SIZE];
}

bool shouldBlock(const KURL& url, int type)
{
    if (!ad_block_enabled) { return false; }
    if (type < 0) { type = DOCUMENT_TYPE; }

    if (!ab_cache) {
        ab_cache = initialize();
    }
    String target = url.string();
    CacheEntry& ent = ab_cache[ab_hash(target) % CACHE_SIZE];
    if (ent.target != target) {
        ent.target = target;
        ent.block = (!ab_whiteList.matches(target, type))
                   && ab_blackList.matches(target, type);
    }
    return ent.block;
}

}
