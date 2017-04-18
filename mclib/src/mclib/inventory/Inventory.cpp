#include <mclib/inventory/Inventory.h>

#include <mclib/protocol/packets/PacketDispatcher.h>

namespace mc {
namespace inventory {

const s32 Inventory::HOTBAR_SLOT_START = 36;
const s32 Inventory::PLAYER_INVENTORY_ID = 0;

Inventory::Inventory(core::Connection* connection, int windowId)
    : m_WindowId(windowId),
      m_Connection(connection),
      m_SelectedHotbarIndex(0)
{

}

Slot Inventory::GetItem(s32 index) const {
    auto iter = m_Items.find(index);
    if (iter == m_Items.end()) return Slot();
    return iter->second;
}

s32 Inventory::FindItemById(s32 itemId) const {
    auto iter = std::find_if(m_Items.begin(), m_Items.end(), [&](const std::pair<s32, Slot>& slot) {
        return slot.second.GetItemId() == itemId;
    });

    if (iter == m_Items.end()) return -1;
    return iter->first;
}

bool Inventory::Contains(s32 itemId) const {
    auto iter = std::find_if(m_Items.begin(), m_Items.end(), [&](const std::pair<s32, Slot>& slot) {
        const Slot& compare = slot.second;

        return compare.GetItemId() == itemId;
    });

    return iter != m_Items.end();
}

bool Inventory::Contains(Slot item) const {
    auto iter = std::find_if(m_Items.begin(), m_Items.end(), [&](const std::pair<s32, Slot>& slot) {
        const Slot& compare = slot.second;

        return compare.GetItemId() == item.GetItemId() && 
               compare.GetItemDamage() == item.GetItemDamage();
    });

    return iter != m_Items.end();
}

bool Inventory::ContainsAtLeast(s32 itemId, s32 amount) const {
    auto iter = std::find_if(m_Items.begin(), m_Items.end(), [&](const std::pair<s32, Slot>& slot) {
        const Slot& compare = slot.second;

        return compare.GetItemId() == itemId &&
               compare.GetItemCount() >= amount;
    });

    return iter != m_Items.end();
}

bool Inventory::ContainsAtLeast(Slot item, s32 amount) const {
    auto iter = std::find_if(m_Items.begin(), m_Items.end(), [&](const std::pair<s32, Slot>& slot) {
        const Slot& compare = slot.second;

        return compare.GetItemId() == item.GetItemId() &&
               compare.GetItemDamage() == item.GetItemDamage() && 
               compare.GetItemCount() >= amount;
    });

    return iter != m_Items.end();
}

InventoryManager::InventoryManager(protocol::packets::PacketDispatcher* dispatcher, core::Connection* connection)
    : protocol::packets::PacketHandler(dispatcher),
      m_Connection(connection)
{
    using namespace protocol;
    dispatcher->RegisterHandler(State::Play, play::SetSlot, this);
    dispatcher->RegisterHandler(State::Play, play::WindowItems, this);
}

InventoryManager::~InventoryManager() {
    GetDispatcher()->UnregisterHandler(this);
}

Inventory* InventoryManager::GetInventory(s32 windowId) {
    auto iter = m_Inventories.find(windowId);
    if (iter == m_Inventories.end()) return nullptr;
    return iter->second.get();
}

Inventory* InventoryManager::GetPlayerInventory() {
    return GetInventory(Inventory::PLAYER_INVENTORY_ID);
}

void InventoryManager::SetSlot(s32 windowId, s32 slotIndex, const Slot& slot) {
    auto iter = m_Inventories.find(windowId);

    Inventory* inventory = nullptr;
    if (iter == m_Inventories.end()) {
        auto newInventory = std::make_unique<Inventory>(m_Connection, windowId);
        inventory = newInventory.get();
        m_Inventories[windowId] = std::move(newInventory);
    } else {
        inventory = iter->second.get();
    }

    inventory->m_Items[slotIndex] = slot;
}

void InventoryManager::HandlePacket(protocol::packets::in::SetSlotPacket* packet) {
    SetSlot(packet->GetWindowId(), packet->GetSlotIndex(), packet->GetSlot());
}

void InventoryManager::HandlePacket(protocol::packets::in::WindowItemsPacket* packet) {
    const std::vector<Slot>& slots = packet->GetSlots();

    for (std::size_t i = 0; i < slots.size(); ++i) {
        SetSlot(packet->GetWindowId(), i, slots[i]);
    }
}

} // ns inventory
} // ns mc
