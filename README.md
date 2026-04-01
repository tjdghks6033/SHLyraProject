# SHLyraProject

> Lyra Starter Game (UE5) 기반 커스텀 Game Feature Plugin

---

## 개요

본 프로젝트는 Unreal Engine 5의 **Lyra Starter Game**을 기반으로,
**라이라 소스를 일절 수정하지 않고** `GameFeaturePlugin` 시스템을 통해 기능을 확장한 포트폴리오입니다.

### 핵심 설계 원칙
- 라이라 코어 코드 **무수정 (Zero Modification)**
- `GameFeatureAction`으로 컴포넌트 / 어빌리티 / UI 동적 주입
- `GAS (Gameplay Ability System)` 기반 전투 및 상태 관리
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
| 커스텀 캐릭터 컴포넌트 | GameFeatureAction으로 라이라 캐릭터에 동적 부착 | 예정 |
| GAS 어빌리티 | 커스텀 어빌리티 세트 및 이펙트 | 예정 |
| 커스텀 UI | CommonUI 기반 위젯 슬롯 확장 | 예정 |
| Enhanced Input | 커스텀 InputConfig 매핑 | 예정 |

---

## 아키텍처

```
SHLyraProject (GameFeaturePlugin)
    │
    ├── GameFeatureData (에셋)
    │       └── GameFeatureActions
    │               ├── AddComponents   → 라이라 캐릭터에 SHHealthComponent 부착
    │               ├── AddAbilities    → LyraAbilitySet에 커스텀 어빌리티 추가
    │               └── AddWidgets      → CommonUI 레이어에 커스텀 HUD 주입
    │
    └── Source/SHLyraProjectRuntime/
            ├── Character/      (USHHealthComponent 등)
            ├── Abilities/      (USHGameplayAbility 등)
            └── UI/             (USHHUDWidget 등)
```

라이라의 `ExperienceDefinition`이 로드되면 `GameFeatureData`의 Action들이 자동 실행되어
기존 라이라 액터에 기능이 주입됩니다.
