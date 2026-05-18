# SHLyraProject

> Lyra Starter Game (UE5) 기반 커스텀 Game Feature Plugin — 포트폴리오 프로젝트

---

## 개요

Unreal Engine 5의 **Lyra Starter Game**을 기반으로,
**라이라 소스를 일절 수정하지 않고** `GameFeaturePlugin` 시스템을 통해 기능을 확장한 프로젝트입니다.

플레이어는 얼음(하늘색) 속성의 근접·마법 어빌리티로 싸우고,
보스는 화염(주황) 속성의 BehaviorTree 기반 AI로 거리와 HP에 따라 패턴을 전환합니다.

---

## 핵심 설계 원칙

- **Zero Modification** — `Source/LyraGame/` 및 에픽 플러그인 파일 무수정. 모든 기능은 `GameFeatureAction` 주입
- **GAS 완전 활용** — 커스텀 `AttributeSet`(Stamina, Mana), `GameplayAbility`, `GameplayEffect`, `GameplayCue` 파이프라인 구성
- **Lyra 아키텍처 준수** — `ULyraExperienceDefinition`, `ULyraAbilitySet`, `LyraDamageExecution` 등 라이라 기존 시스템과 통합
- **보스 AI** — `BehaviorTree` + 거리/HP 기반 Phase 패턴, `BTDecorator` 커스텀 C++
- **Replication 고려** — `AttributeSet` 복제, `GameplayMessage` 브로드캐스트, CMC 예측 호환 이동

---

## 구동 방법

```
1. Epic Games Launcher에서 Lyra Starter Game 5.x 설치
2. Plugins/GameFeatures/ 폴더에 이 레포 클론
   git clone https://github.com/tjdghks6033/SHLyraProject
3. LyraStarterGame.uproject 우클릭 → Generate Visual Studio project files
4. 언리얼 에디터 실행 → L_SH_Expanse 맵을 열어 PIE 실행
```

---

## 구현 시스템

### 플레이어 어빌리티

| 시스템 | 설명 |
|--------|------|
| `GA_SHMeleeAttack` | 몽타주 + AnimNotify 히트 판정 + WeaponSocket Sweep Trace + GAS 비용/쿨다운 |
| `GA_SHDash` | 4방향 입력 판정 + RootMotionConstantForce (CMC 예측 호환) + 스태미나 연동 |
| `GA_SHIceBoltProjectile` | 캐스팅 몽타주 + AnimNotify 스폰 + ProjectileMovement + LyraDamageExecution |
| `GA_SHFireballProjectile` | 화염 발사체 어빌리티 (E 슬롯), Niagara VFX 적용 |

### 리소스 시스템 (GAS AttributeSet)

| 시스템 | 설명 |
|--------|------|
| `USHStaminaSet` / `USHStaminaComponent` | Stamina/MaxStamina/StaminaCost 어트리뷰트, Regen GE 최적화, OutOfStamina 태그 관리 |
| `USHManaSet` / `USHManaComponent` | Mana/MaxMana 어트리뷰트, OutOfMana 태그, 마법 어빌리티 발동 차단 |

### 보스 시스템

| 시스템 | 설명 |
|--------|------|
| `ASHEnemyBoss` + `ASHBossSpawner` | Experience 로드 후 보스 스폰, PawnData 수동 주입, ASC 초기화 직접 호출 |
| `ASHBossController` + `BT_SHBoss` | BehaviorTree 기반 AI — HP/거리 조건으로 Phase 1/2 패턴 전환 |
| `BTDecorator_SHCheckDistance` | 보스↔플레이어 거리 비교 커스텀 C++ Decorator |
| `BTTask_SHActivateAbility` | `TryActivateAbilitiesByTag` 래핑 범용 BT 노드, `bWaitForAbilityEnd` latent 옵션 |
| `GA_SHBossMelee` | AnimNotify(`Event.SH.Boss.HitDetect`) + `hand_r` 소켓 SphereTrace + GE 적용 |

### HUD / UI

| 시스템 | 설명 |
|--------|------|
| `W_SHMeleeHUDLayout` | `LyraHUDLayout` 상속, ShooterCore 없이 독립 동작, UIExtension 슬롯 구성 |
| `WBP_SHSkillBar` / `WBP_SHSkillSlot` | Q/W/E/R 스킬 액션바 — 방사형 쿨다운 오버레이, `Effect Tag Query` 방식 |
| `WBP_SHBossHealthBar` | `SH.Message.Boss.Engaged` 구독 → 보스 액터 바인딩, 실시간 HP 갱신 |
| Floating Damage Numbers | `GCNL_Character_DamageTaken` + `B_NiagaraNumberPopComponent` (Lyra 내장 파이프라인) |

### 기반 시스템

| 시스템 | 설명 |
|--------|------|
| `DA_SHMeleeExperience` | `ULyraExperienceDefinition` 기반 독립 게임모드 — ShooterCore 의존 없음 |
| `ASHEnemyBase` / Bot 계층 | `ALyraCharacter` 기반, `ILyraTeamAgentInterface` 직접 구현, 팀 컬러 MID 주입 |
| CyberSword 팀 컬러 머티리얼 | `Team.WeaponTint` Vector Parameter — Desaturate + Multiply로 Ice(하늘색)/Fire(주황) 분리 |

---

## 아키텍처

```
SHLyraProject (GameFeaturePlugin)
    │
    ├── DA_SHMeleeExperience (독립 Experience)
    │       ├── DefaultPawnData → DA_SHPawnData → BP_SHCharacter
    │       ├── GameFeaturesToEnable → ["SHLyraProject"]   (ShooterCore 미포함)
    │       └── ActionSets
    │               ├── LAS_SHMelee_SharedInput        — IMC_Default + IMC_SHMelee
    │               ├── LAS_SHMelee_StandardComponents — StaminaComponent, ManaComponent 부착
    │               └── LAS_SHMelee_StandardHUD        — HUDLayout + 전체 위젯 UIExtension 주입
    │
    ├── ASHBossSpawner (Experience 로드 후 보스 스폰)
    │       ├── PawnExtComp::SetPawnData (Possess 전 수동 주입)
    │       ├── InitializeAbilitySystem 직접 호출 (LyraHeroComponent 부재 우회)
    │       └── GE_SHBossInitialize_Health (Healing 속성으로 초기 HP 설정)
    │
    └── Source/SHLyraProjectRuntime/
            ├── Character/
            │       ├── SHStaminaComponent   — Regen GE 최적화, OutOfStamina 태그
            │       └── SHManaComponent      — Regen GE 최적화, OutOfMana 태그
            ├── AbilitySystem/
            │       ├── Attributes/
            │       │       ├── SHStaminaSet — Stamina / MaxStamina / StaminaCost
            │       │       └── SHManaSet    — Mana / MaxMana
            │       └── Abilities/
            │               ├── SHMeleeAttack       — 몽타주 + AnimNotify + SweepTrace
            │               ├── SHDash              — RootMotionConstantForce
            │               ├── SHMagicProjectile   — IceBolt 발사체
            │               ├── SHFireballProjectile— Fireball 발사체
            │               └── SHBossMeleeAttack   — 보스 근접 + SphereTrace
            ├── Enemy/
            │       ├── SHEnemyBase  — ALyraCharacter 상속, 팀 색상 MID 주입, 사망 처리
            │       ├── SHEnemyBot   — 졸개 봇
            │       └── SHEnemyBoss  — 보스 (Phase 관리, GE 초기화)
            ├── AI/
            │       ├── SHEnemyControllerBase — AAIController + ILyraTeamAgentInterface
            │       ├── SHBotController       — Tick 기반 MoveToActor
            │       └── SHBossController      — BehaviorTree, OnHealthChanged → BB_HPPercent
            └── Teams/
                    └── SHLyraProjectTeamIds  — Team 0 (Player) / Team 10 (Enemy)
```

### GAS 주입 흐름

```
게임 시작
  → ALyraPlayerState 생성 → ASC 초기화 → NAME_LyraAbilityReady
  → AddAbilities 발동
      → DA_SHAbilitySet      : USHStaminaSet, USHManaSet ASC 등록
      → DA_SHCombatAbilitySet: GA_SHMeleeAttack, GA_SHDash, GA_SHIceBoltProjectile 등 부여
  → LyraPawnExtensionComponent::OnAbilitySystemInitialized
  → USHStaminaComponent / USHManaComponent 바인딩 완료
```

### 보스 AI 패턴

```
BT_SHBoss
└── Selector
    ├── [Decorator: HPPercent ≤ 0.5]  Phase 2 (쿨다운 단축)
    │   ├── [Decorator: Distance < 250]  BTTask_SHActivateAbility(Boss.Melee)
    │   └── BTTask_SHActivateAbility(Boss.Fireball)
    └── Phase 1 (기본)
        ├── [Decorator: Distance < 250]  BTTask_SHActivateAbility(Boss.Melee)
        └── BTTask_SHActivateAbility(Boss.Fireball)
```

---

## 주요 기술 결정

**AttributeSet 메타 어트리뷰트 패턴**
`StaminaCost`는 메타 어트리뷰트로, GE가 소비량을 기록하면 `PostGameplayEffectExecute`에서 `Stamina`를 차감 후 0으로 초기화합니다. Instant GE에서 `Stamina`를 직접 조작하면 클램핑 로직을 우회할 수 있어 이 패턴을 채택했습니다.

**ASC 위치 — PlayerState**
Lyra의 ASC는 리스폰 후 스탯 유지를 위해 `ALyraCharacter`가 아닌 `ALyraPlayerState`에 존재합니다. `GameFeatureAction_AddAbilities`의 Actor Class를 `ALyraPlayerState`로 지정해야 올바르게 동작합니다.

**보스 ASC 초기화 우회**
`LyraGameMode::RestartPlayer` 없이 수동으로 보스를 스폰하면 `ULyraHeroComponent`가 없어 ASC 초기화 체인이 끊깁니다. `ASHBossSpawner`에서 `PawnExtComp->InitializeAbilitySystem(LyraPS_ASC, LyraPS)`를 직접 호출해 해결했습니다.

**LyraDamageExecution 파이프라인**
`Healing` 속성 직접 조작 방식은 `Lyra.Damage.Message`를 브로드캐스트하지 않아 Floating Damage Numbers가 동작하지 않습니다. `LyraCombatSet.BaseDamage`에 Execution Calculation Modifier를 사용하는 공식 파이프라인으로 통일했습니다.
