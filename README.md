# SHLyraProject

> Lyra Starter Game (UE5) 기반 커스텀 Game Feature Plugin

---

## 개요

Unreal Engine 5의 **Lyra Starter Game**을 기반으로,
**라이라 소스를 일절 수정하지 않고** `GameFeaturePlugin` 시스템을 통해 기능을 확장한 프로젝트입니다.

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
```

---

## 구현 시스템

| 시스템 | 설명 | 상태 |
|--------|------|------|
| USHHealthComponent | LyraHealthSet 이벤트 구독, 저체력 GameplayTag 관리, GameplayMessage 브로드캐스트 | 완료 |
| USHStaminaSet | Lyra에 없는 스태미나 리소스를 GAS AttributeSet으로 직접 설계, 복제 지원 | 완료 |
| USHStaminaComponent | 스태미나 상태 관리, Regen GE 최적화, OutOfStamina 태그 관리, 대쉬 스태미나 소비 및 차단 | 완료 |
| GA_SHDash | 스태미나를 소비하는 대시 어빌리티 | 예정 |
| WBP_SHStaminaBar | 스태미나 HUD 위젯 (CommonUI + GameplayMessage) | 진행 중 |

---

## 아키텍처

```
SHLyraProject (GameFeaturePlugin)
    │
    ├── GameFeatureData
    │       └── GameFeatureActions
    │               ├── AddComponents  → ALyraCharacter에 USHHealthComponent, USHStaminaComponent 부착
    │               ├── AddAbilities   → ALyraPlayerState에 DA_SHAbilitySet 부여 (USHStaminaSet 포함)
    │               └── AddWidgets     → UIExtension 슬롯에 커스텀 HUD 주입 (예정)
    │
    └── Source/SHLyraProjectRuntime/
            ├── Character/
            │       ├── SHHealthComponent    — LyraHealthSet 구독, SH.Status.LowHealth 태그
            │       └── SHStaminaComponent   — StaminaSet 바인딩, Regen GE 관리
            └── AbilitySystem/
                    └── Attributes/
                            └── SHStaminaSet — Stamina / MaxStamina / StaminaCost 어트리뷰트
```

### GameFeature 주입 흐름

```
게임 시작
  → ALyraPlayerState 생성 → ASC 초기화 완료 → NAME_LyraAbilityReady 이벤트
  → AddAbilities 발동 → DA_SHAbilitySet → USHStaminaSet이 ASC에 등록
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

### 스태미나 회복 최적화

`GE_SHStaminaRegen` (Infinite + Periodic) 은 `USHStaminaComponent`가 직접 관리합니다.

- 스태미나가 최대치 미만일 때만 GE를 활성화
- 스태미나가 가득 차면 GE를 제거해 불필요한 Periodic 틱을 차단
- 스태미나가 소비되면 GE를 재적용

### GameplayTag

| 태그 | 부여 시점 | 용도 |
|------|----------|------|
| `SH.Status.LowHealth` | 체력 30% 이하 | 위험 상태 연출 트리거 |
| `SH.Status.OutOfStamina` | 스태미나 0 | GA_SHDash 발동 차단 |

### GameplayMessage 채널

| 채널 | 페이로드 | 구독 대상 |
|------|---------|---------|
| `SH.Message.Health.Changed` | `FSHHealthChangedMessage` | 체력 HUD |
| `SH.Message.Stamina.Changed` | `FSHStaminaChangedMessage` | 스태미나 HUD |
