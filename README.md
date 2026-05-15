# SHLyraProject

> Lyra Starter Game (UE5) 기반 커스텀 Game Feature Plugin

---

## 개요

Unreal Engine 5의 **Lyra Starter Game**을 기반으로,
**라이라 소스를 일절 수정하지 않고** `GameFeaturePlugin` 시스템을 통해 기능을 확장한 포트폴리오 프로젝트입니다.

### 핵심 설계 원칙
- 라이라 코어 코드 **무수정 (Zero Modification)**
- `GameFeatureAction`으로 컴포넌트 / 어빌리티 / UI 동적 주입
- `GAS (Gameplay Ability System)` 기반 스태미나 리소스 설계
- 멀티플레이어를 고려한 `Replication` 설계

---

## 구동 방법

```
1. Epic Games Launcher에서 Lyra Starter Game 5.x 설치
2. 프로젝트 루트의 Plugins/GameFeatures/ 폴더에 이 레포 클론
   git clone https://github.com/[your-id]/SHLyraProject
3. LyraStarterGame.uproject 우클릭 → Generate Visual Studio project files
4. 언리얼 에디터 실행
5. Edit > Plugins > SHLyraProject 활성화
6. L_SH_Expanse 맵을 열어 PIE 실행
```

---

## 구현 시스템

| 시스템 | 설명 | 상태 |
|--------|------|------|
| `USHStaminaSet` | Lyra에 없는 스태미나 리소스를 GAS AttributeSet으로 직접 설계, 복제 지원 | 완료 |
| `USHStaminaComponent` | 스태미나 상태 관리, Regen GE 최적화, OutOfStamina 태그 관리, 대쉬 스태미나 소비 및 차단 | 완료 |
| `WBP_SHStaminaBar` | 스태미나 HUD 위젯 — CommonUI + GameplayMessage 기반, 4종 애니메이션 | 완료 |
| `GA_SHMeleeAttack` | C++ 근접 공격 어빌리티 — 몽타주 + AnimNotify 히트 판정 + WeaponSocket Sweep Trace + GAS 비용/쿨다운 파이프라인 | 완료 |
| `GA_SHDash` | C++ 4방향 대쉬 어빌리티 — 입력 방향 판정 + RootMotion(CMC 예측 호환) + 스태미나/쿨다운 연동 | 완료 |
| `DA_SHMeleeExperience` | `ULyraExperienceDefinition` 기반 독립 게임모드 — ShooterCore 의존 없는 완전 독립 Experience | 완료 |
| `W_SHMeleeHUDLayout` | `LyraHUDLayout` 상속 커스텀 HUD 레이아웃 — ShooterCore HUD 없이 독립 동작 | 완료 |
| `BP_SHCharacter` | `B_Hero_Default` 상속 커스텀 캐릭터 — SKM_Manny + 사이버 검 부착 + 소드 트레일 VFX | 완료 |
| `ASHEnemyBase` / AI 계층 | `ALyraCharacter` 기반 Base/Bot/Boss 계층 + `AAIController` + `ILyraTeamAgentInterface` 직접 구현 봇 컨트롤러 계층 (Lyra 내부 `ALyraPlayerBotController`가 외부 모듈 C++ 상속 불가하여 동등 역할 재현) | 진행 중 |
| `GA_SHIceBoltProjectile` | C++ 마법 발사체 어빌리티 — 캐스팅 몽타주 + AnimNotify 스폰 + ProjectileMovement + LyraDamageExecution 파이프라인 | 완료 |
| `USHManaSet` | 마법 어빌리티 자원 — GAS AttributeSet(Mana/MaxMana/ManaCost), 복제 지원 | 완료 |
| `USHManaComponent` | 마나 상태 관리, Regen GE 최적화, OutOfMana 태그, `Ability.Type.Action.Magic` 차단 | 완료 |
| `WBP_SHManaBar` | 마나 HUD 위젯 — `SH.Message.Mana.Changed` 구독, 스태미나 바 패턴 동일 | 완료 |
| `WBP_SHSkillBar` / `WBP_SHSkillSlot` | Q/W/E/R 스킬 액션바 — 쿨다운 방사형 오버레이, UIExtension 주입 | 완료 |
| Floating Damage Numbers | `GCNL_Character_DamageTaken` + `B_NiagaraNumberPopComponent` — Lyra 내장 파이프라인 활용, `LyraDamageExecution` → `Lyra.Damage.Message` 브로드캐스트 연동 | 완료 |
| `GA_SHFireballProjectile` | 화염 발사체 어빌리티 — E 슬롯 프로토타입, Free Magic VFX 적용 | 완료 |
| CyberSword 팀 컬러 머티리얼 | `Team.WeaponTint` 파라미터 추가 — Desaturate + 팀컬러 Multiply로 텍스처 형태 유지하며 Ice(하늘색)/Fire(주황) 분리, `BP_SHEnemyBoss` WeaponMesh 부착 | 완료 |

---

## 아키텍처

```
SHLyraProject (GameFeaturePlugin)
    │
    ├── GameFeatureData (SHLyraProject)
    │       └── GameFeatureActions
    │               ├── AddComponents  → ALyraCharacter에 USHStaminaComponent 부착
    │               ├── AddAbilities   → ALyraPlayerState에 DA_SHAbilitySet 부여 (USHStaminaSet 포함)
    │               │                  → ALyraPlayerState에 DA_SHCombatAbilitySet 부여 (GA_SHMeleeAttack, GA_SHDash 포함)
    │               ├── AddInputContextMapping → IMC_SHMelee (근접 공격 입력)
    │               ├── AddInputBind          → InputData_SHMelee_AddOns
    │               └── AddWidgets     → UIExtension 슬롯에 WBP_SHStaminaBar 주입
    │
    ├── DA_SHMeleeExperience (독립 게임모드)
    │       ├── DefaultPawnData → DA_SHPawnData → BP_SHCharacter
    │       ├── GameFeaturesToEnable → ["SHLyraProject"] (ShooterCore 미포함)
    │       └── ActionSets
    │               ├── LAS_SHMelee_SharedInput       — IMC_Default + IMC_SHMelee
    │               ├── LAS_SHMelee_StandardComponents — BP_SHStaminaComponent 부착
    │               └── LAS_SHMelee_StandardHUD        — W_SHMeleeHUDLayout + WBP_SHStaminaBar
    │
    └── Source/SHLyraProjectRuntime/
            ├── Character/
            │       └── SHStaminaComponent   — StaminaSet 바인딩, Regen GE 관리, 대쉬 감지
            ├── AbilitySystem/
            │       ├── Attributes/
            │       │       └── SHStaminaSet — Stamina / MaxStamina / StaminaCost 어트리뷰트
            │       └── Abilities/
            │               ├── SHMeleeAttack — 몽타주 + AnimNotify + Sweep Trace + GE 적용
            │               └── SHDash        — 4방향 판정 + RootMotionConstantForce
            ├── Enemy/
            │       ├── SHEnemyBase           — ALyraCharacter 상속, 팀 색상 MID 주입, 사망 지연 파괴
            │       ├── SHEnemyBot            — 졸개 (확장점)
            │       └── SHEnemyBoss           — 보스 (뼈대, Phase 관리 예정)
            ├── AI/
            │       ├── SHEnemyControllerBase — AAIController + ILyraTeamAgentInterface 직접 구현
            │       ├── SHBotController       — Tick 기반 MoveToActor
            │       └── SHBossController      — BehaviorTree 기반 (뼈대)
            └── Teams/
                    └── SHLyraProjectTeamIds  — Team 0 (Player) / Team 10 (SHEnemy) 상수
```

### GameFeature 주입 흐름

```
게임 시작
  → ALyraPlayerState 생성 → ASC 초기화 완료 → NAME_LyraAbilityReady 이벤트
  → AddAbilities 발동 → DA_SHAbilitySet → USHStaminaSet이 ASC에 등록
  →                   → DA_SHCombatAbilitySet → GA_SHMeleeAttack, GA_SHDash 부여
  → LyraPawnExtensionComponent → OnAbilitySystemInitialized 발화
  → USHStaminaComponent 바인딩 완료
```

> `AddAbilities`의 Actor Class는 `ALyraPlayerState`로 지정해야 합니다.
> Lyra의 ASC는 리스폰 후 스탯 유지를 위해 Character가 아닌 PlayerState에 존재합니다.

---

## GAS 설계

### USHStaminaSet 어트리뷰트

| 어트리뷰트 | 설명 | 복제 |
|-----------|------|------|
| `Stamina` | 현재 스태미나 (0 ~ MaxStamina) | O |
| `MaxStamina` | 최대 스태미나 | O |
| `StaminaCost` | 메타 어트리뷰트 — GE가 소비량을 기록, PostGE에서 Stamina 차감 후 0 초기화 | X |

### Gameplay Effects

| 이름 | 타입 | 효과 |
|------|------|------|
| `GE_SHStaminaRegen` | Infinite + Periodic (1초) | Stamina +5 회복 |
| `GE_SHStaminaDashCost` | Instant | StaminaCost 메타 어트리뷰트로 50 차감 |
| `GE_SHMeleeDamage` | Instant | LyraHealthSet.Damage 메타 어트리뷰트 적용 |
| `GE_SHMeleeStaminaCost` | Instant | StaminaCost 메타 어트리뷰트로 스태미나 차감 |
| `GE_SHMeleeCooldown` | Duration 0.8초 | `Cooldown.SH.Melee.Attack` 태그 부여 |
| `GE_SHDashCooldown` | Duration | `Cooldown.SH.Dash` 태그 부여 |

### 스태미나 회복 최적화

`GE_SHStaminaRegen` (Infinite + Periodic) 은 `USHStaminaComponent`가 직접 관리합니다.

- 스태미나가 최대치 미만일 때만 GE를 활성화
- 스태미나가 가득 차면 GE를 제거해 불필요한 Periodic 틱을 차단
- 스태미나가 소비되면 GE를 재적용

### GameplayTag

| 태그 | 부여 시점 | 용도 |
|------|----------|------|
| `SH.Status.OutOfStamina` | 스태미나 < 50 | GA_SHDash 발동 차단 |
| `Cooldown.SH.Melee.Attack` | 근접 공격 직후 | 쿨다운 중 재발동 차단 |
| `Cooldown.SH.Dash` | 대쉬 직후 | 쿨다운 중 재발동 차단 |

### GameplayMessage 채널

| 채널 | 페이로드 | 구독 대상 |
|------|---------|---------|
| `SH.Message.Stamina.Changed` | `FSHStaminaChangedMessage` | WBP_SHStaminaBar |

---

## 근접 공격 파이프라인

```
[입력] → GA_SHMeleeAttack 발동
    → CommitAbility (스태미나 소비 + 쿨다운 시작)
    → AbilityTask_PlayMontageAndWait — AM_Melee_Combo01_All_IP 재생
    → AN_SH_MeleeHitDetect AnimNotify → Event.SH.Melee.HitDetect 발화
    → AbilityTask_WaitGameplayEvent 수신
    → PerformMeleeHit: WeaponHilt → WeaponTip 소켓 Sphere Sweep
      (소켓 없으면 캐릭터 전방 구체 트레이스 폴백)
    → GE_SHMeleeDamage → LyraHealthSet.Damage 적용
    → NS_Trail_04 Niagara 소드 트레일 (AnimNotifyState 구간)
```

---

## 예정 작업

| 시스템 | 설명 |
|--------|------|
| Enemy 폴리싱 | 사망 애니메이션, 적 비주얼 구분, 피격 반응, Floating Damage Number |
| 보스 캐릭터 | AI BehaviorTree + 보스 전용 AbilitySet + 스폰 시스템 |
| `GA_SHIceBoltProjectile` | C++ 발사체 어빌리티 — 캐스팅 몽타주 + Projectile Actor + 충돌 데미지 |
