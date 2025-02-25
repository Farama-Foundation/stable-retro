---
hide-toc: true
firstpage:
lastpage:
---

```{project-logo} _static/img/stable-retro-text.png
:alt: Stable-Retro Logo
```

```{project-heading}
Retro games for Reinforcement Learning
```

```{figure} _static/img/retro_games.png
   :alt: Stable-retro gif
   :width: 500
```

**Stable-Retro is a maintained fork of OpenAI’s Retro library.**

stable-retro lets you turn classic video games into Gymnasium environments for reinforcement learning. Supported plateforms includes Sega Genesis, Sega 32X, Super Nintendo, Atari 2600, Arcade Machines and more ([full list here](https://github.com/Farama-Foundation/stable-retro#emulated-systems))

```{code-block} python
"""
Train an agent using Proximal Policy Optimization from Stable Baselines 3
"""

import argparse

import gymnasium as gym
import numpy as np
from gymnasium.wrappers.time_limit import TimeLimit
from stable_baselines3 import PPO
from stable_baselines3.common.atari_wrappers import ClipRewardEnv, WarpFrame
from stable_baselines3.common.vec_env import (
    SubprocVecEnv,
    VecFrameStack,
    VecTransposeImage,
)

import retro


class StochasticFrameSkip(gym.Wrapper):
    def __init__(self, env, n, stickprob):
        gym.Wrapper.__init__(self, env)
        self.n = n
        self.stickprob = stickprob
        self.curac = None
        self.rng = np.random.RandomState()
        self.supports_want_render = hasattr(env, "supports_want_render")

    def reset(self, **kwargs):
        self.curac = None
        return self.env.reset(**kwargs)

    def step(self, ac):
        terminated = False
        truncated = False
        totrew = 0
        for i in range(self.n):
            # First step after reset, use action
            if self.curac is None:
                self.curac = ac
            # First substep, delay with probability=stickprob
            elif i == 0:
                if self.rng.rand() > self.stickprob:
                    self.curac = ac
            # Second substep, new action definitely kicks in
            elif i == 1:
                self.curac = ac
            if self.supports_want_render and i < self.n - 1:
                ob, rew, terminated, truncated, info = self.env.step(
                    self.curac,
                    want_render=False,
                )
            else:
                ob, rew, terminated, truncated, info = self.env.step(self.curac)
            totrew += rew
            if terminated or truncated:
                break
        return ob, totrew, terminated, truncated, info


def make_retro(*, game, state=None, max_episode_steps=4500, **kwargs):
    if state is None:
        state = retro.State.DEFAULT
    env = retro.make(game, state, **kwargs)
    env = StochasticFrameSkip(env, n=4, stickprob=0.25)
    if max_episode_steps is not None:
        env = TimeLimit(env, max_episode_steps=max_episode_steps)
    return env


def wrap_deepmind_retro(env):
    """
    Configure environment for retro games, using config similar to DeepMind-style Atari in openai/baseline's wrap_deepmind
    """
    env = WarpFrame(env)
    env = ClipRewardEnv(env)
    return env


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", default="Airstriker-Genesis")
    parser.add_argument("--state", default=retro.State.DEFAULT)
    parser.add_argument("--scenario", default=None)
    args = parser.parse_args()

    def make_env():
        env = make_retro(game=args.game, state=args.state, scenario=args.scenario)
        env = wrap_deepmind_retro(env)
        return env

    venv = VecTransposeImage(VecFrameStack(SubprocVecEnv([make_env] * 8), n_stack=4))
    model = PPO(
        policy="CnnPolicy",
        env=venv,
        learning_rate=lambda f: f * 2.5e-4,
        n_steps=128,
        batch_size=32,
        n_epochs=4,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.1,
        ent_coef=0.01,
        verbose=1,
    )
    model.learn(
        total_timesteps=100_000_000,
        log_interval=1,
    )


if __name__ == "__main__":
    main()
```

Currently supported games:
```
1942-Nes                                          DragonTheBruceLeeStory-Genesis                        PorkyPigsHauntedHoliday-Snes
1943-Nes                                          DragonTheBruceLeeStory-Snes                           PoseidonWars3D-Sms
3NinjasKickBack-Genesis                           DragonsLair-Snes                                      PowerAthlete-Genesis
8Eyes-Nes                                         DragonsRevenge-Genesis                                PowerPiggsOfTheDarkAge-Snes
AaahhRealMonsters-Genesis                         DreamTeamUSA-Genesis                                  PowerStrike-Sms
AbadoxTheDeadlyInnerWar-Nes                       DynamiteDuke-Genesis                                  PowerStrikeII-Sms
AcceleBrid-Snes                                   DynamiteDuke-Sms                                      Predator2-Genesis
ActRaiser2-Snes                                   DynamiteHeaddy-Genesis                                Predator2-Sms
ActionPachio-Snes                                 ESPNBaseballTonight-Genesis                           PrehistorikMan-Snes
AddamsFamily-GameBoy                              EarnestEvans-Genesis                                  PrivateEye-Atari2600
AddamsFamily-Genesis                              EarthDefenseForce-Snes                                PsychicWorld-Sms
AddamsFamily-Nes                                  ElViento-Genesis                                      Pulseman-Genesis
AddamsFamily-Sms                                  ElementalMaster-Genesis                               PunchOut-Nes
AddamsFamily-Snes                                 ElevatorAction-Atari2600                              Punisher-Genesis
AddamsFamilyPugsleysScavengerHunt-Nes             ElevatorAction-Nes                                    Punisher-Nes
AddamsFamilyPugsleysScavengerHunt-Snes            EliminateDown-Genesis                                 PussNBootsPerosGreatAdventure-Nes
AdvancedBusterhawkGleylancer-Genesis              Enduro-Atari2600                                      PuttySquad-Snes
Adventure-Atari2600                               EuropeanClubSoccer-Genesis                            QBert-Nes
AdventureIsland-GameBoy                           ExMutants-Genesis                                     Qbert-Atari2600
AdventureIsland3-Nes                              Exerion-Nes                                           QuackShot-Genesis
AdventureIslandII-Nes                             F1-Genesis                                            Quartet-Sms
AdventuresOfBatmanAndRobin-Genesis                FZSenkiAxisFinalZone-Genesis                          Quarth-Nes
AdventuresOfBayouBilly-Nes                        FaeryTaleAdventure-Genesis                            RType-Sms
AdventuresOfDinoRiki-Nes                          FamilyDog-Snes                                        RTypeIII-Snes
AdventuresOfDrFranken-Snes                        Fantasia-Genesis                                      RadicalRex-Genesis
AdventuresOfKidKleets-Snes                        FantasticDizzy-Genesis                                RadicalRex-Snes
AdventuresOfMightyMax-Genesis                     FantasyZone-Sms                                       RaidenDensetsu-Snes
AdventuresOfMightyMax-Snes                        FantasyZoneIIOpaOpaNoNamida-Nes                       RaidenDensetsuRaidenTrad-Genesis
AdventuresOfRockyAndBullwinkleAndFriends-Genesis  FantasyZoneIITheTearsOfOpaOpa-Sms                     RainbowIslands-Nes
AdventuresOfRockyAndBullwinkleAndFriends-Nes      FantasyZoneTheMaze-Sms                                RainbowIslandsStoryOfTheBubbleBobble2-Sms
AdventuresOfRockyAndBullwinkleAndFriends-Snes     FatalFury-Genesis                                     RamboFirstBloodPartII-Sms
AdventuresOfStarSaver-GameBoy                     FatalFury2-Genesis                                    RamboIII-Genesis
AdventuresOfYogiBear-Snes                         FatalLabyrinth-Genesis                                Rampage-Nes
AeroFighters-Snes                                 FatalRewind-Genesis                                   RangerX-Genesis
AeroStar-GameBoy                                  FelixTheCat-Nes                                       RastanSagaII-Genesis
AeroTheAcroBat-Snes                               FerrariGrandPrixChallenge-Genesis                     Realm-Snes
AeroTheAcroBat2-Genesis                           FightingMasters-Genesis                               RenAndStimpyShowPresentsStimpysInvention-Genesis
AeroTheAcroBat2-Snes                              FinalBubbleBobble-Sms                                 RenAndStimpyShowVeediots-Snes
AfterBurnerII-Genesis                             FinalFight-Snes                                       RenderingRangerR2-Snes
AfterBurst-GameBoy                                FinalFight2-Snes                                      Renegade-Nes
AirBuster-Genesis                                 FinalFight3-Snes                                      Renegade-Sms
AirCavalry-Snes                                   FinalFightGuy-Snes                                    RevengeOfShinobi-Genesis
AirDiver-Genesis                                  FireAndIce-Sms                                        RiseOfTheRobots-Genesis
AirRaid-Atari2600                                 FirstSamurai-Snes                                     RiskyWoods-Genesis
Airstriker-Genesis                                FishingDerby-Atari2600                                Ristar-Genesis
Airwolf-Nes                                       FistOfTheNorthStar-Nes                                RivalTurf-Snes
AlfredChicken-GameBoy                             Flicky-Genesis                                        Riverraid-Atari2600
AlfredChicken-Nes                                 Flintstones-Genesis                                   RoadRunner-Atari2600
AlfredChicken-Snes                                Flintstones-Snes                                      RoadRunnersDeathValleyRally-Snes
Alien-Atari2600                                   FlintstonesTheRescueOfDinoAndHoppy-Nes                RoboCop2-Nes
Alien3-Nes                                        FlyingDragonTheSecretScroll-Nes                       RoboCop3-Genesis
Alien3-Sms                                        FlyingHero-Nes                                        RoboCop3-Nes
AlienSoldier-Genesis                              FlyingHeroBugyuruNoDaibouken-Snes                     RoboCop3-Sms
AlienSyndrome-Sms                                 ForemanForReal-Genesis                                RoboCop3-Snes
AlienVsPredator-Snes                              ForgottenWorlds-Genesis                               RoboCopVersusTheTerminator-Sms
Alleyway-GameBoy                                  FormationZ-Nes                                        RoboCopVersusTheTerminator-Snes
AlphaMission-Nes                                  FoxsPeterPanAndThePiratesTheRevengeOfCaptainHook-Nes  RoboWarrior-Nes
AlteredBeast-Genesis                              FrankThomasBigHurtBaseball-Genesis                    RoboccoWars-Nes
Amagon-Nes                                        Freeway-Atari2600                                     Robotank-Atari2600
AmazingPenguin-GameBoy                            Frogger-Genesis                                       RocketKnightAdventures-Genesis
AmericanGladiators-Genesis                        FrontLine-Nes                                         RockinKats-Nes
Amidar-Atari2600                                  Frostbite-Atari2600                                   Rollerball-Nes
ArchRivalsTheArcadeGame-Genesis                   FushigiNoOshiroPitPot-Sms                             Rollergames-Nes
ArcherMacleansSuperDropzone-Snes                  GIJoeARealAmericanHero-Nes                            RollingThunder2-Genesis
ArdyLightfoot-Snes                                GIJoeTheAtlantisFactor-Nes                            RunSaber-Snes
Argus-Nes                                         GadgetTwins-Genesis                                   RunningBattle-Sms
ArielTheLittleMermaid-Genesis                     Gaiares-Genesis                                       RushnAttack-Nes
Arkanoid-Nes                                      GainGround-Genesis                                    SCATSpecialCyberneticAttackTeam-Nes
ArkistasRing-Nes                                  GalagaDemonsOfDeath-Nes                               SDHeroSoukessenTaoseAkuNoGundan-Nes
Armadillo-Nes                                     GalaxyForce-Sms                                       Sagaia-Genesis
ArrowFlash-Genesis                                GalaxyForceII-Genesis                                 Sagaia-Sms
ArtOfFighting-Genesis                             Gauntlet-Genesis                                      SaintSword-Genesis
ArtOfFighting-Snes                                Gauntlet-Sms                                          SameSameSame-Genesis
Assault-Atari2600                                 Geimos-Nes                                            SamuraiShodown-Genesis
Asterix-Atari2600                                 GeneralChaos-Genesis                                  Sansuu5And6NenKeisanGame-Nes
Asterix-Sms                                       GhostsnGoblins-Nes                                    Satellite7-Sms
Asterix-Snes                                      GhoulSchool-Nes                                       Seaquest-Atari2600
AsterixAndObelix-GameBoy                          GhoulsnGhosts-Genesis                                 SecondSamurai-Genesis
AsterixAndTheGreatRescue-Genesis                  Gimmick-Nes                                           SectionZ-Nes
AsterixAndTheGreatRescue-Sms                      GlobalDefense-Sms                                     Seicross-Nes
AsterixAndThePowerOfTheGods-Genesis               GlobalGladiators-Genesis                              SeikimaIIAkumaNoGyakushuu-Nes
AsterixAndTheSecretMission-Sms                    Gods-Genesis                                          SenjouNoOokamiIIMercs-Genesis
Asteroids-Atari2600                               Gods-Snes                                             ShadowDancerTheSecretOfShinobi-Genesis
Asteroids-GameBoy                                 GokujouParodius-Snes                                  ShadowOfTheBeast-Genesis
AstroRabby-GameBoy                                GoldenAxe-Genesis                                     ShaqFu-Genesis
AstroRoboSasa-Nes                                 GoldenAxeIII-Genesis                                  Shatterhand-Nes
AstroWarrior-Sms                                  Gopher-Atari2600                                      Shinobi-Sms
Astyanax-Nes                                      Gradius-Nes                                           ShinobiIIIReturnOfTheNinjaMaster-Genesis
Athena-Nes                                        GradiusII-Nes                                         SilverSurfer-Nes
Atlantis-Atari2600                                GradiusIII-Snes                                       SimpsonsBartVsTheSpaceMutants-Genesis
AtlantisNoNazo-Nes                                GradiusTheInterstellarAssault-GameBoy                 SimpsonsBartVsTheSpaceMutants-Nes
AtomicRoboKid-Genesis                             Granada-Genesis                                       SimpsonsBartVsTheWorld-Nes
AtomicRunner-Genesis                              Gravitar-Atari2600                                    SimpsonsBartmanMeetsRadioactiveMan-Nes
AttackAnimalGakuen-Nes                            GreatCircusMysteryStarringMickeyAndMinnie-Genesis     SkeletonKrew-Genesis
AttackOfTheKillerTomatoes-GameBoy                 GreatCircusMysteryStarringMickeyAndMinnie-Snes        Skiing-Atari2600
AttackOfTheKillerTomatoes-Nes                     GreatTank-Nes                                         SkuljaggerRevoltOfTheWesticans-Snes
AwesomePossumKicksDrMachinosButt-Genesis          GreatWaldoSearch-Genesis                              SkyDestroyer-Nes
Axelay-Snes                                       GreendogTheBeachedSurferDude-Genesis                  SkyKid-Nes
AyrtonSennasSuperMonacoGPII-Genesis               Gremlins2TheNewBatch-Nes                              SkyShark-Nes
BOB-Genesis                                       GrindStormer-Genesis                                  SmartBall-Snes
BOB-Snes                                          Growl-Genesis                                         SmashTV-Nes
BWings-Nes                                        GuardianLegend-Nes                                    Smurfs-Genesis
BackToTheFuturePartIII-Genesis                    GuerrillaWar-Nes                                      Smurfs-Nes
BadDudes-Nes                                      GunNac-Nes                                            Smurfs-Snes
BadStreetBrawler-Nes                              Gunship-Genesis                                       SnakeRattleNRoll-Nes
BakuretsuSenshiWarrior-GameBoy                    Gynoug-Genesis                                        SnowBrothers-Nes
BalloonFight-Nes                                  Gyrodine-Nes                                          Socket-Genesis
BalloonKid-GameBoy                                Gyruss-Nes                                            SolDeace-Genesis
Baltron-Nes                                       HammerinHarry-Nes                                     Solaris-Atari2600
BananaPrince-Nes                                  HangOn-Sms                                            SoldiersOfFortune-Genesis
BanishingRacer-GameBoy                            HardDrivin-Genesis                                    SonSon-Nes
BankHeist-Atari2600                               HarleysHumongousAdventure-Snes                        SonicAndKnuckles-Genesis
Barbie-Nes                                        Havoc-Genesis                                         SonicAndKnuckles3-Genesis
BarkleyShutUpAndJam-Genesis                       HeavyBarrel-Nes                                       SonicBlast-Sms
BarkleyShutUpAndJam2-Genesis                      HeavyNova-Genesis                                     SonicBlastMan-Snes
BarneysHideAndSeekGame-Genesis                    HeavyUnitMegaDriveSpecial-Genesis                     SonicBlastManII-Snes
BartSimpsonsEscapeFromCampDeadly-GameBoy          Hellfire-Genesis                                      SonicTheHedgehog-Genesis
Batman-Genesis                                    HelloKittyWorld-Nes                                   SonicTheHedgehog-Sms
BatmanReturns-Genesis                             Hero-Atari2600                                        SonicTheHedgehog2-Genesis
BatmanReturns-Nes                                 HighStakesGambling-GameBoy                            SonicTheHedgehog2-Sms
BatmanReturns-Snes                                HomeAlone-Genesis                                     SonicTheHedgehog3-Genesis
BattleArenaToshinden-GameBoy                      HomeAlone2LostInNewYork-Genesis                       SonicTheHedgehogRandomLevels-Genesis
BattleBull-GameBoy                                HomeAlone2LostInNewYork-Nes                           SonicWings-Snes
BattleCity-Nes                                    Hook-Genesis                                          SpaceHarrier-Nes
BattleMasterKyuukyokuNoSenshiTachi-Snes           Hook-Snes                                             SpaceHarrier-Sms
BattleSquadron-Genesis                            HuntForRedOctober-Nes                                 SpaceHarrier3D-Sms
BattleTechAGameOfArmoredCombat-Genesis            HuntForRedOctober-Snes                                SpaceHarrierII-Genesis
BattleUnitZeoth-GameBoy                           Hurricanes-Genesis                                    SpaceInvaders-Atari2600
BattleZequeDen-Snes                               Hurricanes-Snes                                       SpaceInvaders-Nes
BattleZone-Atari2600                              IMGInternationalTourTennis-Genesis                    SpaceInvaders-Snes
Battletoads-Genesis                               IceClimber-Nes                                        SpaceInvaders91-Genesis
Battletoads-Nes                                   IceHockey-Atari2600                                   SpaceMegaforce-Snes
BattletoadsDoubleDragon-Genesis                   Ikari-Nes                                             SpankysQuest-Snes
BattletoadsDoubleDragon-Snes                      IkariIIITheRescue-Nes                                 Sparkster-Genesis
BattletoadsInBattlemaniacs-Snes                   Ikki-Nes                                              Sparkster-Snes
BattletoadsInRagnaroksWorld-GameBoy               Imperium-Snes                                         SpartanX2-Nes
BeamRider-Atari2600                               Incantation-Snes                                      SpeedyGonzalesLosGatosBandidos-Snes
BeautyAndTheBeastBellesQuest-Genesis              IncredibleCrashDummies-Genesis                        Spelunker-Nes
BeautyAndTheBeastRoarOfTheBeast-Genesis           IncredibleHulk-Genesis                                SpiderManReturnOfTheSinisterSix-Sms
BebesKids-Snes                                    IncredibleHulk-Sms                                    Splatterhouse2-Genesis
Berzerk-Atari2600                                 IncredibleHulk-Snes                                   SpotGoesToHollywood-Genesis
BillAndTedsExcellentGameBoyAdventure-GameBoy      IndianaJonesAndTheLastCrusade-Genesis                 SprigganPowered-Snes
BiminiRun-Genesis                                 IndianaJonesAndTheTempleOfDoom-Nes                    SpyHunter-Nes
BinaryLand-Nes                                    InsectorX-Genesis                                     Sqoon-Nes
BioHazardBattle-Genesis                           InsectorX-Nes                                         StarForce-Nes
BioMetal-Snes                                     InspectorGadget-Snes                                  StarGunner-Atari2600
BioMiracleBokutteUpa-Nes                          IronSwordWizardsAndWarriorsII-Nes                     StarSoldier-Nes
BioSenshiDanIncreaserTonoTatakai-Nes              IshidoTheWayOfStones-Genesis                          StarWars-Nes
BirdWeek-Nes                                      IsolatedWarrior-Nes                                   StarshipHector-Nes
BishoujoSenshiSailorMoon-Genesis                  ItchyAndScratchyGame-Snes                             SteelEmpire-Genesis
BishoujoSenshiSailorMoonR-Snes                    IzzysQuestForTheOlympicRings-Genesis                  SteelTalons-Genesis
BlaZeonTheBioCyborgChallenge-Snes                 IzzysQuestForTheOlympicRings-Snes                     Stinger-Nes
BladeEagle-Sms                                    Jackal-Nes                                            StoneProtectors-Snes
BladesOfVengeance-Genesis                         JackieChansActionKungFu-Nes                           Stormlord-Genesis
BlockKuzushi-Snes                                 JajamaruNoDaibouken-Nes                               StreetFighterIISpecialChampionEdition-Genesis
BlockKuzushiGB-GameBoy                            JamesBond007TheDuel-Genesis                           StreetSmart-Genesis
Blockout-Genesis                                  JamesBond007TheDuel-Sms                               StreetsOfRage-Genesis
BodyCount-Genesis                                 JamesBondJr-Nes                                       StreetsOfRage2-Genesis
BombJack-GameBoy                                  JamesPond2CodenameRoboCod-Sms                         StreetsOfRage3-Genesis
BomberRaid-Sms                                    JamesPond3-Genesis                                    StreetsOfRageII-Sms
BonkersWaxUp-Sms                                  JamesPondIICodenameRobocod-Genesis                    Strider-Genesis
BoobyBoys-GameBoy                                 JamesPondUnderwaterAgent-Genesis                      SubTerrania-Genesis
BoobyKids-Nes                                     Jamesbond-Atari2600                                   SubmarineAttack-Sms
BoogermanAPickAndFlickAdventure-Genesis           Jaws-Nes                                              SunsetRiders-Genesis
BoogermanAPickAndFlickAdventure-Snes              JellyBoy-Snes                                         SuperAdventureIsland-Snes
BoogieWoogieBowling-Genesis                       JetsonsCogswellsCaper-Nes                             SuperAlfredChicken-Snes
BoulderDash-GameBoy                               JetsonsInvasionOfThePlanetPirates-Snes                SuperArabian-Nes
BoulderDash-Nes                                   JewelMaster-Genesis                                   SuperBCKid-Snes
Bowling-Atari2600                                 JoeAndMac-Genesis                                     SuperC-Nes
Boxing-Atari2600                                  JoeAndMac-Nes                                         SuperCastlevaniaIV-Snes
BoxingLegendsOfTheRing-Genesis                    JoeAndMac-Snes                                        SuperDoubleDragon-Snes
BramStokersDracula-Genesis                        JoeAndMac2LostInTheTropics-Snes                       SuperFantasyZone-Genesis
BramStokersDracula-Nes                            JourneyEscape-Atari2600                               SuperGhoulsnGhosts-Snes
BramStokersDracula-Snes                           JourneyToSilius-Nes                                   SuperHangOn-Genesis
BrawlBrothers-Snes                                Joust-Nes                                             SuperJamesPond-Snes
BreakThru-Nes                                     JuJuDensetsuTokiGoingApeSpit-Genesis                  SuperMarioBros-Nes
Breakout-Atari2600                                JudgeDredd-Genesis                                    SuperMarioBros2Japan-Nes
BronkieTheBronchiasaurus-Snes                     JudgeDredd-Snes                                       SuperMarioBros3-Nes
BubbaNStix-Genesis                                JungleBook-Genesis                                    SuperMarioWorld-Snes
BubbleAndSqueak-Genesis                           JungleBook-Nes                                        SuperMarioWorld2-Snes
BubbleBobble-Nes                                  JungleBook-Snes                                       SuperPitfall-Nes
BubbleBobble-Sms                                  JusticeLeagueTaskForce-Genesis                        SuperRType-Snes
BubbleBobblePart2-Nes                             KaGeKiFistsOfSteel-Genesis                            SuperSWIV-Snes
BubbleGhost-GameBoy                               Kaboom-Atari2600                                      SuperSmashTV-Genesis
BubsyII-Genesis                                   KabukiQuantumFighter-Nes                              SuperSmashTV-Snes
BubsyII-Snes                                      KaiketsuYanchaMaru2KarakuriLand-Nes                   SuperSpaceInvaders-Sms
BubsyInClawsEncountersOfTheFurredKind-Genesis     KaiketsuYanchaMaru3TaiketsuZouringen-Nes              SuperStarForce-Nes
BubsyInClawsEncountersOfTheFurredKind-Snes        KaiteTsukutteAsoberuDezaemon-Snes                     SuperStarWars-Snes
BuckyOHare-Nes                                    KamenNoNinjaAkakage-Nes                               SuperStarWarsReturnOfTheJedi-Snes
BugsBunnyBirthdayBlowout-Nes                      Kangaroo-Atari2600                                    SuperStarWarsTheEmpireStrikesBack-Snes
BullsVersusBlazersAndTheNBAPlayoffs-Genesis       KanshakudamaNageKantarouNoToukaidouGojuusanTsugi-Nes  SuperStrikeGunner-Snes
BullsVsLakersAndTheNBAPlayoffs-Genesis            KeroppiToKeroriinuNoSplashBomb-Nes                    SuperThunderBlade-Genesis
BuraiFighter-Nes                                  KidChameleon-Genesis                                  SuperTrollIslands-Snes
BurningForce-Genesis                              KidIcarus-Nes                                         SuperTurrican-Snes
CacomaKnightInBizyland-Snes                       KidKlownInCrazyChase-Snes                             SuperTurrican2-Snes
Cadash-Genesis                                    KidKlownInNightMayorWorld-Nes                         SuperValisIV-Snes
CalRipkenJrBaseball-Genesis                       KidNikiRadicalNinja-Nes                               SuperWidget-Snes
Caliber50-Genesis                                 KingOfDragons-Snes                                    SuperWonderBoy-Sms
CaliforniaGames-Genesis                           KingOfTheMonsters2-Genesis                            SuperXeviousGumpNoNazo-Nes
Cameltry-Snes                                     KingOfTheMonsters2-Snes                               Superman-Genesis
CannonFodder-Genesis                              KirbysAdventure-Nes                                   SupermanTheManOfSteel-Sms
CannonFodder-Snes                                 KnightsOfTheRound-Snes                                SwampThing-Nes
CaptainAmericaAndTheAvengers-Genesis              Krull-Atari2600                                       SwordOfSodan-Genesis
CaptainAmericaAndTheAvengers-Nes                  KungFu-Nes                                            SydOfValis-Genesis
CaptainAmericaAndTheAvengers-Snes                 KungFuHeroes-Nes                                      SylvesterAndTweetyInCageyCapers-Genesis
CaptainCommando-Snes                              KungFuKid-Sms                                         T2TheArcadeGame-Genesis
CaptainPlanetAndThePlaneteers-Genesis             KungFuMaster-Atari2600                                T2TheArcadeGame-Sms
CaptainPlanetAndThePlaneteers-Nes                 LandOfIllusionStarringMickeyMouse-Sms                 TaiyouNoYuushaFighbird-Nes
CaptainSilver-Nes                                 LastActionHero-Genesis                                TakahashiMeijinNoBugutteHoney-Nes
Carnival-Atari2600                                LastActionHero-Nes                                    TargetEarth-Genesis
Castelian-Nes                                     LastActionHero-Snes                                   TargetRenegade-Nes
CastleOfIllusion-Genesis                          LastBattle-Genesis                                    TaskForceHarrierEX-Genesis
Castlevania-Nes                                   LastStarfighter-Nes                                   TazMania-Genesis
CastlevaniaBloodlines-Genesis                     LawnmowerMan-Genesis                                  TazMania-Sms
CastlevaniaDraculaX-Snes                          Legend-Snes                                           TazMania-Snes
CastlevaniaIIIDraculasCurse-Nes                   LegendOfGalahad-Genesis                               TeenageMutantNinjaTurtles-Nes
CastlevaniaTheNewGeneration-Genesis               LegendOfKage-Nes                                      TeenageMutantNinjaTurtlesIIITheManhattanProject-Nes
CatNindenTeyandee-Nes                             LegendOfPrinceValiant-Nes                             TeenageMutantNinjaTurtlesIITheArcadeGame-Nes
Centipede-Atari2600                               LegendaryWings-Nes                                    TeenageMutantNinjaTurtlesIVTurtlesInTime-Snes
ChacknPop-Nes                                     LethalEnforcers-Genesis                               TeenageMutantNinjaTurtlesTheHyperstoneHeist-Genesis
Challenger-Nes                                    LethalEnforcersIIGunFighters-Genesis                  TeenageMutantNinjaTurtlesTournamentFighters-Genesis
ChampionsWorldClassSoccer-Genesis                 LethalWeapon-Snes                                     TeenageMutantNinjaTurtlesTournamentFighters-Nes
ChampionshipProAm-Genesis                         LifeForce-Nes                                         Tennis-Atari2600
ChaosEngine-Genesis                               LineOfFire-Sms                                        Terminator-Genesis
ChaseHQII-Genesis                                 LittleMermaid-Nes                                     Terminator-Sms
CheeseCatAstropheStarringSpeedyGonzales-Genesis   LowGManTheLowGravityMan-Nes                           Terminator2JudgmentDay-Nes
CheeseCatAstropheStarringSpeedyGonzales-Sms       LuckyDimeCaperStarringDonaldDuck-Sms                  TerraCresta-Nes
ChesterCheetahTooCoolToFool-Genesis               MCKids-Nes                                            TetrastarTheFighter-Nes
ChesterCheetahTooCoolToFool-Snes                  MUSHA-Genesis                                         Tetris-GameBoy
ChesterCheetahWildWildQuest-Genesis               MagicBoy-Snes                                         TetrisAttack-Snes
ChesterCheetahWildWildQuest-Snes                  MagicSword-Snes                                       TetsuwanAtom-Nes
ChiChisProChallengeGolf-Genesis                   MagicalQuestStarringMickeyMouse-Snes                  Thexder-Nes
ChikiChikiBoys-Genesis                            MagicalTaruruutoKun-Genesis                           ThunderAndLightning-Nes
Choplifter-Nes                                    Magmax-Nes                                            ThunderBlade-Sms
ChoplifterIIIRescueSurvive-Snes                   MaouRenjishi-Genesis                                  ThunderForceII-Genesis
ChopperCommand-Atari2600                          MappyLand-Nes                                         ThunderForceIII-Genesis
ChouFuyuuYousaiExedExes-Nes                       MarbleMadness-Genesis                                 ThunderForceIV-Genesis
ChoujikuuYousaiMacross-Nes                        MarbleMadness-Sms                                     ThunderFox-Genesis
ChoujikuuYousaiMacrossScrambledValkyrie-Snes      MarioBros-Nes                                         ThunderSpirits-Snes
ChubbyCherub-Nes                                  Marko-Genesis                                         Thundercade-Nes
ChuckRock-Genesis                                 Marsupilami-Genesis                                   Tick-Genesis
ChuckRock-Sms                                     MarvelLand-Genesis                                    Tick-Snes
ChuckRock-Snes                                    Mask-Snes                                             TigerHeli-Nes
ChuckRockIISonOfChuck-Genesis                     MasterOfDarkness-Sms                                  TimePilot-Atari2600
ChuckRockIISonOfChuck-Sms                         McDonaldsTreasureLandAdventure-Genesis                TimeZone-Nes
CircusCaper-Nes                                   MechanizedAttack-Nes                                  TinHead-Genesis
CircusCharlie-Nes                                 MegaMan-Nes                                           TinyToonAdventures-Nes
CityConnection-Nes                                MegaMan2-Nes                                          TinyToonAdventuresBusterBustsLoose-Snes
ClayFighter-Genesis                               MegaManTheWilyWars-Genesis                            TinyToonAdventuresBustersHiddenTreasure-Genesis
Claymates-Snes                                    MegaSWIV-Genesis                                      Toki-Nes
Cliffhanger-Genesis                               MegaTurrican-Genesis                                  TomAndJerry-Snes
Cliffhanger-Nes                                   MendelPalace-Nes                                      TotalRecall-Nes
Cliffhanger-Snes                                  MetalStorm-Nes                                        TotallyRad-Nes
CloudMaster-Sms                                   MichaelJacksonsMoonwalker-Genesis                     ToxicCrusaders-Genesis
CluCluLand-Nes                                    MichaelJacksonsMoonwalker-Sms                         ToxicCrusaders-Nes
CobraTriangle-Nes                                 MickeyMousecapade-Nes                                 TrampolineTerror-Genesis
CodeNameViper-Nes                                 MidnightResistance-Genesis                            TransBot-Sms
CollegeSlam-Genesis                               MightyBombJack-Nes                                    TreasureMaster-Nes
Columns-Genesis                                   MightyFinalFight-Nes                                  Trog-Nes
ColumnsIII-Genesis                                MightyMorphinPowerRangers-Genesis                     Trojan-Nes
CombatCars-Genesis                                MightyMorphinPowerRangersTheMovie-Genesis             TrollsInCrazyland-Nes
ComicalMachineGunJoe-Sms                          MightyMorphinPowerRangersTheMovie-Snes                TroubleShooter-Genesis
ComixZone-Genesis                                 Millipede-Nes                                         Truxton-Genesis
Conan-Nes                                         MitsumeGaTooru-Nes                                    Turrican-Genesis
CongosCaper-Snes                                  MoeroTwinBeeCinnamonHakaseOSukue-Nes                  Tutankham-Atari2600
ConquestOfTheCrystalPalace-Nes                    MonsterInMyPocket-Nes                                 TwinBee-Nes
ContraForce-Nes                                   MonsterLair-Genesis                                   TwinBee3PokoPokoDaimaou-Nes
CoolSpot-Genesis                                  MonsterParty-Nes                                      TwinCobra-Nes
CoolSpot-Sms                                      MontezumaRevenge-Atari2600                            TwinCobraDesertAttackHelicopter-Genesis
CoolSpot-Snes                                     MoonPatrol-Atari2600                                  TwinEagle-Nes
CosmicEpsilon-Nes                                 MortalKombat-Genesis                                  TwinkleTale-Genesis
CosmoGangTheVideo-Snes                            MortalKombat-SCD                                      TwoCrudeDudes-Genesis
CrackDown-Genesis                                 MortalKombat3-Genesis                                 UNSquadron-Snes
CrazyClimber-Atari2600                            MortalKombatII-Genesis                                UchuuNoKishiTekkamanBlade-Snes
CrossFire-Nes                                     MrNutz-Genesis                                        UndeadLine-Genesis
CrueBallHeavyMetalPinball-Genesis                 MrNutz-Snes                                           UniversalSoldier-Genesis
Curse-Genesis                                     MsPacMan-Genesis                                      Untouchables-Nes
CutieSuzukiNoRingsideAngel-Genesis                MsPacMan-Nes                                          UpNDown-Atari2600
CutthroatIsland-Genesis                           MsPacMan-Sms                                          UrbanChampion-Nes
CyberShinobi-Sms                                  MsPacman-Atari2600                                    UruseiYatsuraLumNoWeddingBell-Nes
Cyberball-Genesis                                 MutantVirusCrisisInAComputerWorld-Nes                 UzuKeobukseon-Genesis
Cybernator-Snes                                   MyHero-Sms                                            VRTroopers-Genesis
CyborgJustice-Genesis                             MysteryQuest-Nes                                      VaporTrail-Genesis
DJBoy-Genesis                                     NARC-Nes                                              Vectorman-Genesis
DaffyDuckInHollywood-Sms                          NHL94-Genesis                                         Vectorman2-Genesis
DaffyDuckTheMarvinMissions-Snes                   NHL941on1-Genesis                                     Venture-Atari2600
DaisenpuuTwinHawk-Genesis                         NHL942on2-Genesis                                     ViceProjectDoom-Nes
DananTheJungleFighter-Sms                         NameThisGame-Atari2600                                VideoPinball-Atari2600
DangerousSeed-Genesis                             NewZealandStory-Genesis                               Viewpoint-Genesis
DariusForce-Snes                                  NewZealandStory-Sms                                   Vigilante-Sms
DariusII-Genesis                                  Ninja-Sms                                             VirtuaFighter-32x
DariusTwin-Snes                                   NinjaCrusaders-Nes                                    VirtuaFighter2-Genesis
DarkCastle-Genesis                                NinjaGaiden-Nes                                       VirtuaFighter2-Saturn
Darkman-Nes                                       NinjaGaiden-Sms                                       VolguardII-Nes
Darwin4081-Genesis                                NinjaGaidenIIITheAncientShipOfDoom-Nes                WWFArcade-Genesis
DashGalaxyInTheAlienAsylum-Nes                    NinjaGaidenIITheDarkSwordOfChaos-Nes                  WaniWaniWorld-Genesis
DashinDesperadoes-Genesis                         NinjaKid-Nes                                          Wardner-Genesis
DavidRobinsonsSupremeCourt-Genesis                NoahsArk-Nes                                          Warpman-Nes
DazeBeforeChristmas-Genesis                       NormysBeachBabeORama-Genesis                          WaynesWorld-Nes
DazeBeforeChristmas-Snes                          OperationWolf-Nes                                     WereBackADinosaursStory-Snes
DeadlyMoves-Genesis                               Ottifants-Genesis                                     WhipRush-Genesis
DeathDuel-Genesis                                 Ottifants-Sms                                         Widget-Nes
DeepDuckTroubleStarringDonaldDuck-Sms             OutToLunch-Snes                                       WizardOfWor-Atari2600
Defender-Atari2600                                OverHorizon-Nes                                       WizardsAndWarriors-Nes
DefenderII-Nes                                    POWPrisonersOfWar-Nes                                 WiznLiz-Genesis
DemonAttack-Atari2600                             PacInTime-Snes                                        Wolfchild-Genesis
DennisTheMenace-Snes                              PacManNamco-Nes                                       Wolfchild-Sms
DesertStrikeReturnToTheGulf-Genesis               PacMania-Genesis                                      Wolfchild-Snes
DevilCrashMD-Genesis                              PacMania-Sms                                          WolverineAdamantiumRage-Genesis
DevilishTheNextPossession-Genesis                 PanicRestaurant-Nes                                   WonderBoyInMonsterWorld-Sms
DickTracy-Genesis                                 Paperboy-Genesis                                      WorldHeroes-Genesis
DickTracy-Sms                                     Paperboy-Nes                                          WrathOfTheBlackManta-Nes
DickVitalesAwesomeBabyCollegeHoops-Genesis        Paperboy-Sms                                          WreckingCrew-Nes
DigDugIITroubleInParadise-Nes                     Paperboy2-Genesis                                     XDRXDazedlyRay-Genesis
DiggerTheLegendOfTheLostCity-Nes                  Parodius-Nes                                          XKaliber2097-Snes
DimensionForce-Snes                               Parodius-Snes                                         XMenMojoWorld-Sms
DinoCity-Snes                                     PeaceKeepers-Snes                                     Xenon2Megablast-Genesis
DinoLand-Genesis                                  PenguinKunWars-Nes                                    Xenophobe-Nes
DirtyHarry-Nes                                    Phalanx-Snes                                          XeviousTheAvenger-Nes
DonDokoDon-Nes                                    Phelios-Genesis                                       Xexyz-Nes
DonkeyKong-Nes                                    Phoenix-Atari2600                                     YarsRevenge-Atari2600
DonkeyKong3-Nes                                   PinkGoesToHollywood-Genesis                           YoukaiClub-Nes
DonkeyKongCountry-Snes                            PiratesOfDarkWater-Snes                               YoukaiDouchuuki-Nes
DonkeyKongCountry2-Snes                           PitFighter-Genesis                                    YoungIndianaJonesChronicles-Nes
DonkeyKongCountry3DixieKongsDoubleTrouble-Snes    PitFighter-Sms                                        Zanac-Nes
DonkeyKongJr-Nes                                  Pitfall-Atari2600                                     Zaxxon-Atari2600
DoubleDragon-Genesis                              PitfallTheMayanAdventure-Genesis                      ZeroTheKamikazeSquirrel-Genesis
DoubleDragon-Nes                                  PitfallTheMayanAdventure-Snes                         ZeroTheKamikazeSquirrel-Snes
DoubleDragonIITheRevenge-Genesis                  PizzaPop-Nes                                          ZeroWing-Genesis
DoubleDragonIITheRevenge-Nes                      Plok-Snes                                             ZombiesAteMyNeighbors-Snes
DoubleDragonVTheShadowFalls-Genesis               Pong-Atari2600                                        ZoolNinjaOfTheNthDimension-Genesis
DoubleDribbleThePlayoffEdition-Genesis            Pooyan-Atari2600                                      ZoolNinjaOfTheNthDimension-Sms
DoubleDunk-Atari2600                              Pooyan-Nes                                            ZoolNinjaOfTheNthDimension-Snes
DrRobotniksMeanBeanMachine-Genesis                Popeye-Nes                                            
DragonPower-Nes                                   PopnTwinBee-Snes
DragonSpiritTheNewLegend-Nes                      PopnTwinBeeRainbowBellAdventures-Snes
```


```{toctree}
:hidden:
:caption: Introduction

getting_started.md
developing.md
integration.md
python.md
```

[//]: # (```{toctree})
[//]: # (:hidden:)
[//]: # (:caption: Environments)
[//]: # ()
[//]: # (```)

```{toctree}
:hidden:
:caption: Development

release_notes.md
Github <https://github.com/Farama-Foundation/stable-retro>
Contribute to the Docs <https://github.com/Farama-Foundation/stable-retro/blob/master/docs/README.md>
```
[//]: # (release_notes/index)
