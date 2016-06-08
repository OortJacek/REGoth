#include "ScriptEngine.h"
#include "PlayerController.h"
#include <daedalus/DaedalusVM.h>
#include <utils/logger.h>
#include <logic/scriptExternals/Externals.h>
#include <logic/scriptExternals/Stubs.h>
#include <daedalus/DaedalusGameState.h>
#include <handle/HandleDef.h>
#include <components/VobClasses.h>
#include <engine/World.h>

using namespace Logic;

ScriptEngine::ScriptEngine(World::WorldInstance& world)
    : m_World(world)
{
    m_pVM = nullptr;
}

ScriptEngine::~ScriptEngine()
{
    delete m_pVM;
}

bool ScriptEngine::loadDAT(const std::string& file)
{
    delete m_pVM; // FIXME: Should support merging DATS?

    LogInfo() << "Loading Daedalus compiled script file: " << file;

    Daedalus::DATFile dat = Daedalus::DATFile(file);
    m_pVM = new Daedalus::DaedalusVM(dat);


    // Register externals
    const bool verbose = false;
    Logic::ScriptExternals::registerStubs(*m_pVM, verbose);
    Logic::ScriptExternals::registerStdLib(*m_pVM, verbose);
    m_pVM->getGameState().registerExternals();
    Logic::ScriptExternals::registerEngineExternals(m_World.getEngine(), m_pVM, verbose);
    return false;
}

void ScriptEngine::prepareRunFunction()
{
    // Clean the VM for this run
    m_pVM->pushState();

    // Init stack with a value of 0. Some functions do not return, and thus, a default must be given.
    pushInt(0);
}


int32_t ScriptEngine::runFunction(const std::string& fname)
{
    assert(m_pVM->getDATFile().hasSymbolName(fname));

    return runFunction(m_pVM->getDATFile().getSymbolByName(fname).address);
}

int32_t ScriptEngine::runFunction(size_t addr)
{
    // Place the call-operation
    m_pVM->doCallOperation(addr);

    // Execute the instructions
    while(m_pVM->doStack());

    int32_t ret = m_pVM->popDataValue();
    // Restore to previous VM-State
    m_pVM->popState();
    return ret;
}

void ScriptEngine::pushInt(int32_t v)
{
    m_pVM->pushInt(v);
}

void ScriptEngine::pushString(const std::string& str)
{
    m_pVM->pushString(str);
}

void ScriptEngine::pushSymbol(size_t sym, uint32_t arrayIndex)
{
    m_pVM->pushVar(sym, arrayIndex);
}

void ScriptEngine::pushSymbol(const std::string& sname)
{
    m_pVM->pushVar(sname);
}

void ScriptEngine::setInstance(const std::string& target, const std::string& source)
{
    // Target is checked later
    assert(m_pVM->getDATFile().hasSymbolName(source));

    setInstance(target,
                m_pVM->getDATFile().getSymbolIndexByName(source));
}

void ScriptEngine::setInstance(const std::string& target, size_t source)
{
    assert(m_pVM->getDATFile().hasSymbolName(target));

    auto& sym = m_pVM->getDATFile().getSymbolByIndex(source);

    m_pVM->setInstance(target, sym.instanceDataHandle, sym.instanceDataClass);
}

void ScriptEngine::initForWorld(const std::string& world)
{
    // Create player
    Daedalus::GameState::NpcHandle hero = m_pVM->getGameState().createNPC();
    m_pVM->initializeInstance(ZMemory::toBigHandle(hero), m_pVM->getDATFile().getSymbolIndexByName("PC_Hero"), Daedalus::IC_Npc);
    m_pVM->setInstance("hero", ZMemory::toBigHandle(hero), Daedalus::IC_Npc);

    // Register our externals
    Daedalus::GameState::DaedalusGameState::GameExternals ext;
    ext.wld_insertnpc = [this](Daedalus::GameState::NpcHandle npc, std::string spawnpoint){ onNPCInserted(npc, spawnpoint); };
    ext.post_wld_insertnpc = [this](Daedalus::GameState::NpcHandle npc){ onNPCInitialized(npc); };
    ext.createinvitem = [this](Daedalus::GameState::ItemHandle item, Daedalus::GameState::NpcHandle npc){ onItemInserted(item, npc); };
    m_pVM->getGameState().setGameExternals(ext);

    // FIXME: Call the startup-one only on a fresh load of the game
    prepareRunFunction();
    runFunction("startup_" + world);

    prepareRunFunction();
    runFunction("init_" + world);
}

void ScriptEngine::onNPCInserted(Daedalus::GameState::NpcHandle npc, const std::string& spawnpoint)
{
    // LogInfo() << "Created npc " << npc.index <<" on: " << spawnpoint;

    // Create the NPC-vob
    Handle::EntityHandle e = VobTypes::initNPCFromScript(m_World, npc);
    Vob::VobInformation v = Vob::asVob(m_World, e);
    m_WorldNPCs.insert(e);


    // Place NPC to it's location
    // TODO: Find some better solution to the casting
    Logic::PlayerController* pc = reinterpret_cast<Logic::PlayerController*>(v.logic);

    //LogInfo() << "Spawnpoint: " << spawnpoint;

    // FIXME: Some waypoints don't seem to exist?
    if(World::Waynet::waypointExists(m_World.getWaynet(), spawnpoint))
        pc->teleportToWaypoint(World::Waynet::getWaypointIndex(m_World.getWaynet(), spawnpoint));
}

Daedalus::GameState::DaedalusGameState& ScriptEngine::getGameState()
{
    return m_pVM->getGameState();
}

size_t ScriptEngine::getSymbolIndexByName(const std::string& name)
{
    return m_pVM->getDATFile().getSymbolIndexByName(name);
}

void ScriptEngine::onItemInserted(Daedalus::GameState::ItemHandle item, Daedalus::GameState::NpcHandle npc)
{
    Daedalus::GEngineClasses::C_Item& itemData = getGameState().getItem(item);
    //LogInfo() << "Inserted item '" << itemData.name
    //          << "' into the inventory of '" << getGameState().getNpc(npc).name[0] << "'";

    // Equip
    // TODO: Implement this properly
    if((itemData.mainflag & Daedalus::GEngineClasses::C_Item::ITM_CAT_ARMOR) != 0)
    {
        //LogInfo() << "Equiping armor... " << itemData.visual_change;
        VobTypes::NpcVobInformation vob = VobTypes::asNpcVob(m_World, VobTypes::getEntityFromScriptInstance(m_World, npc));

        std::string visual = itemData.visual_change.substr(0, itemData.visual_change.size()-4) + ".MDM";
        Vob::setVisual(vob, visual);
    }
}

void ScriptEngine::onNPCInitialized(Daedalus::GameState::NpcHandle npc)
{
    // Initialize daily routine
    Daedalus::GEngineClasses::C_Npc& npcData = getGameState().getNpc(npc);

    //LogInfo() << "Initializing daily routine for: " << npcData.name[0] << ", hdl: " << npc.index;
    //LogInfo() << "Self: " << getSymbolIndexByName("self");


    prepareRunFunction();

    m_pVM->setInstance("self", ZMemory::toBigHandle(npc), Daedalus::IC_Npc);
    m_pVM->setCurrentInstance(getSymbolIndexByName("self"));

    runFunction(npcData.daily_routine);
}


























