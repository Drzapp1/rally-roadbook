# Generate a co-driver voice clip bank (consistent 22050 Hz / 16-bit / mono WAV so
# the plugin can concatenate clips into a spoken pace-note sentence).
param(
  [string]$OutDir = "D:\BikesRoadbook\release\RallyRoadbook\Roadbook_data\audio\voice",
  [string]$Voice  = ""   # leave empty for the default installed voice
)
Add-Type -AssemblyName System.Speech
New-Item -ItemType Directory -Force $OutDir | Out-Null
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(22050, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)

$words = [ordered]@{
  # number building blocks (compose any distance 0-999, plus thousand)
  zero='zero'; one='one'; two='two'; three='three'; four='four'; five='five'; six='six'; seven='seven'; eight='eight'; nine='nine'; ten='ten';
  eleven='eleven'; twelve='twelve'; thirteen='thirteen'; fourteen='fourteen'; fifteen='fifteen'; sixteen='sixteen'; seventeen='seventeen'; eighteen='eighteen'; nineteen='nineteen'; twenty='twenty';
  thirty='thirty'; forty='forty'; fifty='fifty'; sixty='sixty'; seventy='seventy'; eighty='eighty'; ninety='ninety'; hundred='hundred'; thousand='thousand';
  # directions
  left='left'; right='right'; straight='straight'; keep_straight='keep straight'; hairpin='hairpin'; slight='slight'; square='square'; fork='fork'; acute='acute';
  # danger / attention
  caution='caution'; danger='danger'; attention='attention'; careful='careful';
  # features / signs
  crest='crest'; dip='dip'; water='water'; jump='jump'; bridge='bridge'; gate='gate'; narrows='narrows'; rocks='rocks'; bumpy='bumpy';
  junction='junction'; tree='tree'; village='village'; hole='hole'; ruts='ruts'; compression='compression'; sand='sand'; gravel='gravel'; ditch='ditch'; cliff='cliff'; fence='fence'; downhill='downhill';
  # connectors / misc
  into='into'; and='and'; then='then'; over='over'; on_the='on the'; metres='metres'; meters='meters';
  start='start'; finish='finish'; go='go'; now='now'; stop='stop';
}

$ss = New-Object System.Speech.Synthesis.SpeechSynthesizer
if ($Voice -ne "") { try { $ss.SelectVoice($Voice) } catch { Write-Host "voice '$Voice' not found, using default" } }
$ss.Rate = 1   # slightly brisk, like a real co-driver
$n = 0
foreach ($k in $words.Keys) {
  $path = Join-Path $OutDir "$k.wav"
  $ss.SetOutputToWaveFile($path, $fmt)
  $ss.Speak($words[$k])
  $n++
}
$ss.SetOutputToDefaultAudioDevice()
$ss.Dispose()
Write-Host "voice bank: $n clips -> $OutDir"
# trim leading/trailing silence so concatenated pace notes flow at a natural pace
python "$PSScriptRoot\trim_voicebank.py" "$OutDir"
