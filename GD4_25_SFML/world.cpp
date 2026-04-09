#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "pointbox.hpp"
#include "pointbox_type.hpp"
#include "utility.hpp"
#include "sound_node.hpp"
#include "posteffect.hpp"

// Ben Arrowsmith D00257746
// John Nally D00258753

World::World(sf::RenderWindow& window, SoundPlayer& sounds, FontHolder& font, bool networked)
    : m_window(window)
    , m_camera(window.getDefaultView())
    , m_textures()
    , m_sounds(sounds)
    , m_fonts(font)
    , m_scene_graph(ReceiverCategories::kNone)
    , m_scene_layers()
    , m_world_bounds(sf::Vector2f(0.f, 0.f),
        sf::Vector2f(m_camera.getSize().x, 3000.f))
    , m_spawn_position(m_camera.getSize().x / 2.f,
        m_world_bounds.size.y - m_camera.getSize().y / 2.f)
    , m_spawn_position2(m_camera.getSize().x / 2.f + 100.f,
        m_world_bounds.size.y - m_camera.getSize().y / 2.f)
    , m_player_aircraft(nullptr)
    , m_player_aircraft2(nullptr)
    , m_pointbox_spawn_timer(sf::Time::Zero)
    , m_player1_score(0)
    , m_player2_score(0)
    , m_networked(networked)
    , m_network_node(nullptr)
{
    LoadTextures();
    BuildScene();
    m_camera.setCenter(m_spawn_position);
    m_scene_texture.resize({ window.getSize().x, window.getSize().y });
}

// ---------------------------------------------------------------------------
// Update / Draw
// ---------------------------------------------------------------------------

void World::Update(sf::Time dt)
{
    // Reset velocities each frame
    if (m_player_aircraft)  m_player_aircraft->SetVelocity(0.f, 0.f);
    if (m_player_aircraft2) m_player_aircraft2->SetVelocity(0.f, 0.f);

    // In networked mode reset all network aircraft velocities
    for (Aircraft* a : m_network_aircraft)
        a->SetVelocity(0.f, 0.f);

    DestroyEntitiesOutsideView();
    UpdateSounds();

    while (!m_command_queue.IsEmpty())
        m_scene_graph.OnCommand(m_command_queue.Pop(), dt);

    AdaptPlayerVelocity();
    HandleCollisions();
    m_scene_graph.RemoveWrecks();
    m_scene_graph.Update(dt, m_command_queue);
    AdaptPlayerPosition();
    UpdatePointBoxSpawning(dt);
}

void World::Draw()
{
    if (PostEffect::IsSupported())
    {
        m_scene_texture.clear();
        m_scene_texture.setView(m_camera);
        m_scene_texture.draw(m_scene_graph);
        m_scene_texture.display();
        m_bloom_effect.Apply(m_scene_texture, m_window);
    }
    else
    {
        m_window.setView(m_camera);
        m_window.draw(m_scene_graph);
    }
}

CommandQueue& World::GetCommandQueue()
{
    return m_command_queue;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void World::LoadTextures()
{
    m_textures.Load(TextureID::kEagle, "Media/Textures/Player1.png");
    m_textures.Load(TextureID::kEagle2, "Media/Textures/Player2.png");
    m_textures.Load(TextureID::kLandscape, "Media/Textures/background.png");
    m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");

    // John Nally: Point box textures
    m_textures.Load(TextureID::kPointBoxPlusOne, "Media/Textures/box_plus_one.png");
    m_textures.Load(TextureID::kPointBoxPlusTwo, "Media/Textures/box_plus_two.png");
    m_textures.Load(TextureID::kPointBoxPlusThree, "Media/Textures/box_plus_three.png");
    m_textures.Load(TextureID::kPointBoxMinusFive, "Media/Textures/box_minus_five.png");

    // John Nally: Walk animation frames
    m_textures.Load(TextureID::kPlayer1Walk1, "Media/Textures/Player1_Walk1.png");
    m_textures.Load(TextureID::kPlayer1Walk2, "Media/Textures/Player1_Walk2.png");
    m_textures.Load(TextureID::kPlayer2Walk1, "Media/Textures/Player2_Walk1.png");
    m_textures.Load(TextureID::kPlayer2Walk2, "Media/Textures/Player2_Walk2.png");
}

void World::BuildScene()
{
    // Initialise scene layers
    for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
    {
        ReceiverCategories category =
            (i == static_cast<int>(SceneLayers::kAir))
            ? ReceiverCategories::kScene : ReceiverCategories::kNone;
        SceneNode::Ptr layer(new SceneNode(category));
        m_scene_layers[i] = layer.get();
        m_scene_graph.AttachChild(std::move(layer));
    }

    // Background
    sf::Texture& texture = m_textures.Get(TextureID::kLandscape);
    sf::IntRect  textureRect(m_world_bounds);
    texture.setRepeated(true);

    std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
    background_sprite->setPosition(sf::Vector2f(0, 0));
    m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(
        std::move(background_sprite));

    // Sound node
    std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
    m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(soundNode));

    if (m_networked)
    {
        // Networked mode: aircraft are added later via AddAircraft()
        // Plant the NetworkNode in the scene graph
        std::unique_ptr<NetworkNode> network_node(new NetworkNode());
        m_network_node = network_node.get();
        m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(
            std::move(network_node));
    }
    else
    {
        // Local mode: spawn both aircraft immediately (original behaviour)
        std::unique_ptr<Aircraft> leader(new Aircraft(AircraftType::kEagle, m_textures, m_fonts));
        m_player_aircraft = leader.get();
        m_player_aircraft->SetIdentifier(1);
        m_player_aircraft->setPosition(m_spawn_position);
        m_player_aircraft->SetVelocity(0.f, 0.f);
        m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(leader));

        std::unique_ptr<Aircraft> player2(new Aircraft(AircraftType::kEagle2, m_textures, m_fonts));
        m_player_aircraft2 = player2.get();
        m_player_aircraft2->SetIdentifier(2);
        m_player_aircraft2->setPosition(m_spawn_position2);
        m_player_aircraft2->SetVelocity(0.f, 0.f);
        m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(player2));
    }
}

// ---------------------------------------------------------------------------
// Network aircraft management
// ---------------------------------------------------------------------------

Aircraft* World::AddAircraft(uint8_t identifier)
{
    // identifier 1 -> kEagle (Player 1), everything else -> kEagle2 (Player 2)
    AircraftType type = (identifier == 1) ? AircraftType::kEagle : AircraftType::kEagle2;

    std::unique_ptr<Aircraft> aircraft(new Aircraft(type, m_textures, m_fonts));
    aircraft->SetIdentifier(identifier);
    aircraft->setPosition(m_spawn_position);   // server will immediately correct position
    aircraft->SetVelocity(0.f, 0.f);

    Aircraft* ptr = aircraft.get();
    m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(aircraft));
    m_network_aircraft.push_back(ptr);
    return ptr;
}

void World::RemoveAircraft(uint8_t identifier)
{
    Aircraft* aircraft = GetAircraft(identifier);
    if (aircraft)
    {
        aircraft->Destroy();
        m_network_aircraft.erase(
            std::remove(m_network_aircraft.begin(), m_network_aircraft.end(), aircraft),
            m_network_aircraft.end());
    }
}

Aircraft* World::GetAircraft(uint8_t identifier) const
{
    for (Aircraft* a : m_network_aircraft)
        if (a->GetIdentifier() == identifier)
            return a;
    return nullptr;
}

bool World::PollGameAction(GameActions::Action& out)
{
    if (m_network_node)
        return m_network_node->PollGameAction(out);
    return false;
}

// ---------------------------------------------------------------------------
// Player velocity / position clamping
// ---------------------------------------------------------------------------

void World::AdaptPlayerVelocity()
{
    auto normalise = [](Aircraft* a)
        {
            if (!a) return;
            sf::Vector2f v = a->GetVelocity();
            if (v.x != 0.f && v.y != 0.f)
                a->SetVelocity(v / std::sqrt(2.f));
        };

    normalise(m_player_aircraft);
    normalise(m_player_aircraft2);

    for (Aircraft* a : m_network_aircraft)
        normalise(a);
}

void World::AdaptPlayerPosition()
{
    sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f,
        m_camera.getSize());
    const float border = 40.f;

    auto clamp = [&](Aircraft* a)
        {
            if (!a) return;
            sf::Vector2f pos = a->getPosition();
            pos.x = std::min(pos.x, view_bounds.size.x - border);
            pos.x = std::max(pos.x, border);
            pos.y = std::min(pos.y, view_bounds.position.y + view_bounds.size.y - border);
            pos.y = std::max(pos.y, view_bounds.position.y + border);
            a->setPosition(pos);
        };

    clamp(m_player_aircraft);
    clamp(m_player_aircraft2);

    for (Aircraft* a : m_network_aircraft)
        clamp(a);
}

// ---------------------------------------------------------------------------
// Bounds helpers
// ---------------------------------------------------------------------------

sf::FloatRect World::GetViewBounds() const
{
    return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f,
        m_camera.getSize());
}

sf::FloatRect World::GetBattleFieldBounds() const
{
    sf::FloatRect bounds = GetViewBounds();
    bounds.position.y -= 100.f;
    bounds.size.y += 100.f;
    return bounds;
}

// ---------------------------------------------------------------------------
// Collision handling
// ---------------------------------------------------------------------------

static bool MatchesCategories(SceneNode::Pair& colliders,
    ReceiverCategories type1,
    ReceiverCategories type2)
{
    unsigned int cat1 = colliders.first->GetCategory();
    unsigned int cat2 = colliders.second->GetCategory();

    if ((static_cast<int>(type1) & cat1) && (static_cast<int>(type2) & cat2))
        return true;
    if ((static_cast<int>(type1) & cat2) && (static_cast<int>(type2) & cat1))
    {
        std::swap(colliders.first, colliders.second);
        return true;
    }
    return false;
}

void World::HandleCollisions()
{
    std::set<SceneNode::Pair> collision_pairs;
    m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

    for (SceneNode::Pair pair : collision_pairs)
    {
        // ---- Player 1 hit by enemy projectile → -1 score ----
        if (MatchesCategories(pair,
            ReceiverCategories::kPlayerAircraft,
            ReceiverCategories::kEnemyProjectile))
        {
            auto& aircraft = static_cast<Aircraft&>(*pair.first);
            auto& projectile = static_cast<Projectile&>(*pair.second);
            aircraft.AddScore(-1);
            if (!m_networked) m_player1_score -= 1;
            projectile.Destroy();
        }
        // ---- Player 2 hit by allied projectile → -1 score ----
        else if (MatchesCategories(pair,
            ReceiverCategories::kPlayer2Aircraft,
            ReceiverCategories::kAlliedProjectile))
        {
            auto& aircraft = static_cast<Aircraft&>(*pair.first);
            auto& projectile = static_cast<Projectile&>(*pair.second);
            aircraft.AddScore(-1);
            if (!m_networked) m_player2_score -= 1;
            projectile.Destroy();
        }
        // ---- Player 1 collects point box ----
        else if (MatchesCategories(pair,
            ReceiverCategories::kPlayerAircraft,
            ReceiverCategories::kPointBox))
        {
            auto& player = static_cast<Aircraft&>(*pair.first);
            auto& pointbox = static_cast<PointBox&>(*pair.second);
            int pts = pointbox.GetPointValue();
            player.AddScore(pts);
            if (!m_networked) m_player1_score += pts;
            player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
            pointbox.Destroy();
        }
        // ---- Player 2 collects point box ----
        else if (MatchesCategories(pair,
            ReceiverCategories::kPlayer2Aircraft,
            ReceiverCategories::kPointBox))
        {
            auto& player = static_cast<Aircraft&>(*pair.first);
            auto& pointbox = static_cast<PointBox&>(*pair.second);
            int pts = pointbox.GetPointValue();
            player.AddScore(pts);
            if (!m_networked) m_player2_score += pts;
            player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
            pointbox.Destroy();
        }
    }
}

// ---------------------------------------------------------------------------
// Out-of-view cleanup
// ---------------------------------------------------------------------------

void World::DestroyEntitiesOutsideView()
{
    Command command;
    command.category = static_cast<int>(ReceiverCategories::kProjectile)
        | static_cast<int>(ReceiverCategories::kPointBox);
    command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
        {
            if (GetBattleFieldBounds().findIntersection(e.GetBoundingRect()) == std::nullopt)
                e.Destroy();
        });
    m_command_queue.Push(command);
}

// ---------------------------------------------------------------------------
// Point box spawning  (John Nally)
// ---------------------------------------------------------------------------

void World::UpdatePointBoxSpawning(sf::Time dt)
{
    if (m_networked) return; // In networked mode, point box spawning is controlled by the server

    const sf::Time kSpawnInterval = sf::seconds(1.f);
    m_pointbox_spawn_timer += dt;
    if (m_pointbox_spawn_timer >= kSpawnInterval)
    {
        SpawnPointBoxes();
        m_pointbox_spawn_timer = sf::Time::Zero;
    }
}

void World::SpawnPointBoxes()
{
    int random_type = Utility::RandomInt(static_cast<int>(PointBoxType::kPointBoxCount));
    PointBoxType type = static_cast<PointBoxType>(random_type);

    std::unique_ptr<PointBox> box(new PointBox(type, m_textures, m_fonts));

    sf::FloatRect view_bounds = GetViewBounds();
    float min_x = view_bounds.position.x + 50.f;
    float max_x = view_bounds.position.x + view_bounds.size.x - 50.f;
    int   range = static_cast<int>(max_x - min_x);
    float spawn_x = min_x + static_cast<float>(Utility::RandomInt(range + 1));
    float spawn_y = view_bounds.position.y - 50.f;

    box->setPosition(sf::Vector2f(spawn_x, spawn_y));
    m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(box));
}

void World::SpawnNetworkPointBox(uint8_t type_idx, float spawn_x)
{
    PointBoxType type = static_cast<PointBoxType>(type_idx);
    std::unique_ptr<PointBox> box(new PointBox(type, m_textures, m_fonts));
    sf::FloatRect view_bounds = GetViewBounds();
    float spawn_y = view_bounds.position.y - 50.f;
    box->setPosition(sf::Vector2f(spawn_x, spawn_y));
    m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(box));
}

// ---------------------------------------------------------------------------
// Score getters / win condition  (local mode only)
// ---------------------------------------------------------------------------

int World::GetPlayer1Score() const { return m_player1_score; }
int World::GetPlayer2Score() const { return m_player2_score; }

int World::GetWinningPlayer() const
{
    if (m_player1_score > m_player2_score) return 1;
    if (m_player2_score > m_player1_score) return 2;
    return 0;
}

bool World::HasPlayerReachedPoints() const
{
    if (m_networked)
    {
        // In networked mode scores live on the Aircraft objects
        for (Aircraft* a : m_network_aircraft)
            if (a->GetScore() >= 30)
                return true;
        return false;
    }
    // Local mode: use the Aircraft score fields (clamped to 0, same as original)
    return (m_player_aircraft && m_player_aircraft->GetScore() >= 30)
        || (m_player_aircraft2 && m_player_aircraft2->GetScore() >= 30);
}

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

void World::UpdateSounds()
{
    m_sounds.SetListenerPosition(m_camera.getCenter());
    m_sounds.RemoveStoppedSounds();
}
