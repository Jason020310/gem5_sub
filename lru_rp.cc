/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Daniel Carvalho
 */

#include "mem/cache/replacement_policies/lru_rp.hh"

#include <cassert>
#include <memory>

#include "params/LRURP.hh"
#include "mem/cache/blk.hh"

LRURP::LRURP(const Params *p)
    : BaseReplacementPolicy(p)
{
}

void
LRURP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
const
{
    // Reset last touch timestamp
    std::static_pointer_cast<LRUReplData>(
        replacement_data)->lastTouchTick = Tick(0);
}

void
LRURP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Update last touch timestamp
    std::static_pointer_cast<LRUReplData>(
        replacement_data)->lastTouchTick = curTick();
}

void
LRURP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Set last touch timestamp
    std::static_pointer_cast<LRUReplData>(
        replacement_data)->lastTouchTick = curTick();
}
ReplaceableEntry*
LRURP::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    ReplaceableEntry* oldest_clean_entry = nullptr;
    ReplaceableEntry* oldest_dirty_entry = nullptr;
    
    // 使用一個不可能達到的未來時間作為初始時間戳
    Tick oldest_clean_tick = MaxTick; 
    Tick oldest_dirty_tick = MaxTick;

    // 遍歷所有候選者，將它們分類並找到各分類中的LRU
    for (const auto& candidate : candidates) {
        // 取得候選者的時間戳
        Tick candidate_tick = std::static_pointer_cast<LRUReplData>(
            candidate->replacementData)->lastTouchTick;

        // 檢查 Block 是否為 Dirty
        // 我們需要將 ReplaceableEntry* 安全地轉換為 CacheBlk*
        // 在 gem5 的快取實作中，這是安全的轉換
        if (static_cast<CacheBlk*>(candidate)->isDirty()) {
            // 這是 Dirty Block
            if (candidate_tick < oldest_dirty_tick) {
                oldest_dirty_tick = candidate_tick;
                oldest_dirty_entry = candidate;
            }
        } else {
            // 這是 Clean Block
            if (candidate_tick < oldest_clean_tick) {
                oldest_clean_tick = candidate_tick;
                oldest_clean_entry = candidate;
            }
        }
    }

    // 決策：優先淘汰 Clean Block
    if (oldest_clean_entry != nullptr) {
        // 如果找到了 Clean Block，就淘汰最久未使用的那一個
        return oldest_clean_entry;
    } else {
        // 如果所有候選者都是 Dirty 的，才淘汰最久未使用的 Dirty Block
        // 這種情況下 oldest_dirty_entry 必定不為 nullptr
        assert(oldest_dirty_entry != nullptr);
        return oldest_dirty_entry;
    }
}
std::shared_ptr<ReplacementData>
LRURP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new LRUReplData());
}

LRURP*
LRURPParams::create()
{
    return new LRURP(this);
}
