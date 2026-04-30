# DreamLipSync

`DreamLipSync` 是一个基于 Morph Target 的口型同步插件，支持在 Sequencer 中播放，也支持运行时通过组件调用播放。

## 核心资产

创建 `DreamLipSyncClip` DataAsset，用同一份资产同时服务 Sequencer 和 Runtime。

默认 Viseme 映射适配当前卡通模型的 MorphTarget 命名：

| Viseme | MorphTarget |
| --- | --- |
| Neutral | Fcl_MTH_Neutral |
| A | Fcl_MTH_A |
| I | Fcl_MTH_I |
| U | Fcl_MTH_U |
| E | Fcl_MTH_E |
| O | Fcl_MTH_O |
| Close | Fcl_MTH_Close |

`DreamLipSyncClip` 支持两种数据模式：

| 模式 | 用途 |
| --- | --- |
| Viseme Keys | 按时间写 `A/I/U/E/O/Close/Neutral`，插件自动插值并映射到 MorphTarget。 |
| Morph Frames | 直接按时间写多个 MorphTarget 权重，适合外部工具导出的曲线。 |

常用时间参数：

| 字段 | 说明 |
| --- | --- |
| Playback Blend Time | `Viseme Keys` 播放时嘴型之间的混合时间。值越小切换越利落，值越大越柔和。 |
| Generation Blend Time | MFA / Rhubarb 生成 `MorphFrames` 时使用的嘴型过渡时间。 |

### Viseme Recipes

`Viseme Recipes` 用来在识别出的基础嘴型上叠加其他嘴型或直接叠加 MorphTarget。它适合处理卡通模型里“单一 O 太死”“E/I 太硬”“A 需要带一点下唇”等问题。

例如让 `O` 同时带一点 `U` 和 `I`：

```text
Source Viseme = O
bIncludeBaseViseme = true
BaseScale = 1.0
VisemeMixes:
  U = 0.20
  I = 0.10
ExtraMorphWeights:
  Fcl_MTH_Up = 0.05
```

执行顺序是：

```text
MFA/Rhubarb 识别出 O
-> Viseme Recipes 展开成 O + U + I + Extra Morph
-> Viseme Mappings 映射到 Fcl_MTH_*
-> 写入 MorphFrames
```

注意：`Viseme Recipes` 会影响 `Generate From MFA`、`Generate From Rhubarb` 和 `Viseme Keys` 播放；如果 Clip 已经生成过 `MorphFrames`，修改 Recipe 后需要重新生成。

## Sequencer 使用

1. 在 Level Sequence 中绑定角色 Actor 或 SkeletalMeshComponent。
2. 在绑定对象上添加 `Dream Lip Sync Track`。
3. 点击 Track 右侧 `LipSync` 按钮添加 Section。
4. 在 Section Details 中设置 `Clip`。
5. Audio Track 仍然使用 UE 原生音频轨道，和 LipSync Section 对齐即可。

Section 的时间控制：

| 字段 | 说明 |
| --- | --- |
| Auto Size To Clip Duration | 设置 Clip 后自动把 Section 长度改成 Clip 的有效时长。 |
| Lock Section Length To Clip Duration | 防止把 Section 拉得超过 Clip 时长。 |
| Clamp Evaluation To Clip Duration | 即使 Section 被拉长，超过 Clip 时长后也会停止评估并重置嘴型。 |
| Stretch Clip To Section | 把 Clip 时间拉伸到整个 Section 长度，适合刻意放慢/加长口型。 |

右键 Lip Sync Section 可以执行：

```text
Set Length To Clip Duration
```

## 离线生成口型

插件支持三种非商业优先的离线流程：

| 方案 | 适合场景 | 说明 |
| --- | --- | --- |
| MFA | 中文/多语言台词，已知文本 | 用 Montreal Forced Aligner 生成音素级 TextGrid，再映射到 `A/I/U/E/O/Close/Neutral`。 |
| Audio2Face JSON | 需要更自然的连续曲线 | 用 Audio2Face 在外部生成 blendshape 权重，再导入到 `MorphFrames`。 |
| Rhubarb | 快速占位、英文或简单音频 | 直接从 `.wav` / `.ogg` 生成 mouth cues，质量通常低于 MFA/Audio2Face。 |

所有生成流程都会把 `DataMode` 切到 `MorphFrames`，并覆盖当前 `MorphFrames`。

### Clip 需要设置

| 字段 | 说明 |
| --- | --- |
| Sound | 对应音频。必须是从原始音频导入的 `SoundWave`，或者设置 `Source Audio File Override`。 |
| Locale | 例如 `zh-CN`、`en-US`、`ja-JP`。 |
| Source Audio File Override | 当 SoundWave 找不到原始导入文件时，手动指定音频文件。 |
| Dialogue Text | MFA 必填；Rhubarb 可选。填入与音频一致的台词文本。 |

### 推荐：MFA 中文/多语言强制对齐

MFA 不直接“猜嘴型”，它会根据音频和台词文本对齐到音素时间轴。中文、多国语言项目通常比 Rhubarb 的纯音频/粗略口型更稳定。

1. 安装 Montreal Forced Aligner，并安装对应语言模型。例如中文可使用 `mandarin_china_mfa` dictionary + `mandarin_mfa` acoustic model。

   Windows 推荐创建 Python 3.10 环境，中文分词依赖在该版本上更稳：

   ```powershell
   conda create -n aligner310 -c conda-forge python=3.10 montreal-forced-aligner
   conda activate aligner310
   python -m pip install spacy-pkuseg dragonmapper hanziconv
   mfa model download acoustic mandarin_mfa
   mfa model download dictionary mandarin_china_mfa
   where mfa
   ```

   如果使用中文模型但缺少分词依赖，MFA 会报：

   ```text
   Please install Chinese tokenization support via `pip install spacy-pkuseg dragonmapper hanziconv`
   ```
2. 打开：

   ```text
   Project Settings -> DreamPlugin -> Dream Lip Sync
   ```

3. 设置：

   | 设置项 | 说明 |
   | --- | --- |
   | Mfa Command | 默认 `mfa`。如果环境变量找不到，就填 `aligner310\Scripts\mfa.exe` 的完整路径。 |
   | Mfa Root Directory | MFA 模型和缓存目录。C 盘空间紧张时建议放到其他盘，例如 `I:/MFA/Data`。 |
   | Mfa Dictionary | 中文普通话推荐 `mandarin_china_mfa`，也可以填自定义词典路径。 |
   | Mfa Acoustic Model | 默认 `mandarin_mfa`，也可以填自定义模型路径。 |
   | Mfa Num Jobs | 并行任务数。 |
   | Mfa Extra Arguments | 追加到 `mfa align` 的原始高级参数。常用参数建议优先用下面的明确字段。 |
   | Blend Time | 音素切换过渡时间，默认 `0.04` 秒。 |

   常用 MFA 高级参数也已暴露到 `MFA|Advanced`：

   | 设置项 | 说明 |
   | --- | --- |
   | Mfa Config Path | 使用 MFA YAML 配置文件。 |
   | Mfa Speaker Characters | 按文件名前 N 个字符识别说话人，或填 `prosodylab`。 |
   | Mfa G2P Model Path | 给 OOV 词使用的 G2P 模型。 |
   | Clean / Overwrite / Quiet / Verbose | 对应 MFA 的清理、覆盖、日志输出选项。 |
   | Use Multiprocessing / Use Threading | 控制 MFA 并行方式。 |
   | Single Speaker | 单说话人模式，适合单条对白素材。 |
   | No Tokenization | 禁用预训练分词；中文普通话通常保持关闭。 |
   | TextGrid Cleanup | 输出 TextGrid 后处理。 |
   | Fine Tune | 额外边界微调，耗时更长。 |

4. 在 Content Browser 中右键 `DreamLipSyncClip`：

   ```text
   Generate From MFA
   Import MFA TextGrid
   ```

也可以在 Sequencer 中右键 `Dream Lip Sync Track`：

```text
Generate Clips From MFA
```

### Audio2Face JSON 导入

Audio2Face 可以生成更连续的面部 blendshape 曲线。插件不绑定商业云服务，只导入本地 JSON。

1. 在 Audio2Face 中导出包含 blendshape 名称和权重矩阵的 JSON。
2. 在 `DreamLipSyncClip.SourceMorphMappings` 中把外部通道映射到当前模型 MorphTarget。
3. 在 Content Browser 中右键 `DreamLipSyncClip`：

   ```text
   Import Audio2Face JSON
   ```

默认 `SourceMorphMappings` 已包含一组常见 Audio2Face/ARKit 风格名称：

| Source | MorphTarget |
| --- | --- |
| jawOpen | Fcl_MTH_A |
| mouthClose | Fcl_MTH_Close |
| mouthFunnel | Fcl_MTH_O |
| mouthPucker | Fcl_MTH_U |
| mouthStretchLeft / mouthStretchRight | Fcl_MTH_E |
| mouthUpperUpLeft / mouthUpperUpRight | Fcl_MTH_Up |
| mouthLowerDownLeft / mouthLowerDownRight | Fcl_MTH_Down |

### 配置 Rhubarb

1. 下载 Rhubarb Lip Sync。
2. 打开：

   ```text
   Project Settings -> DreamPlugin -> Dream Lip Sync
   ```

3. 设置：

   | 设置项 | 说明 |
   | --- | --- |
   | Rhubarb Executable Path | `rhubarb.exe` 路径。 |
   | Rhubarb Recognizer | `Auto` 会按 Clip 的 `Locale` 选择。`en-*` 使用 PocketSphinx，其他语言使用 Phonetic。 |
   | Extended Shapes | 默认 `GHX`。 |
   | Blend Time | cue 切换过渡时间，默认 `0.04` 秒。 |

### 生成方式

在 Content Browser 中右键 `DreamLipSyncClip`：

```text
Generate From Rhubarb
Import Rhubarb JSON
```

也可以在 Sequencer 中右键 `Dream Lip Sync Track`：

```text
Generate Clips From Rhubarb
```

## Runtime 使用

给角色添加 `DreamLipSyncComponent`：

```text
DreamLipSyncComponent
-> TargetMesh: 角色脸部 SkeletalMeshComponent
-> DefaultClip: DreamLipSyncClip
```

蓝图调用：

```text
Play Lip Sync Clip(Clip, bPlayAudio = true)
Stop Lip Sync()
Set Lip Sync Playback Time()
```

如果 `DreamLipSyncClip.Sound` 已设置，Runtime 播放时可以同步播放音频。

## 多语言建议

每种语言使用独立 Clip 和音频：

```text
Dialogue_001_zh_CN: Audio + DreamLipSyncClip
Dialogue_001_en_US: Audio + DreamLipSyncClip
Dialogue_001_ja_JP: Audio + DreamLipSyncClip
```

所有语言都可以映射到同一套 MorphTarget。
