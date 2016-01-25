local json = require("json")
local gourgandine = require("gourgandine")

-------------------------------------------
-- Extraction
-------------------------------------------

local function check(test)
   local function identical(t1, t2)
      if #t1 ~= #t2 then
         return false
      end
      for i = 1, #t1 do
         if t1[i] ~= t2[i] then
            return false
         end
      end
      return true
   end
   local rec = gourgandine.new()
   local ret = rec:extract(test.input:gsub("\n%s*", " "), test.language or "en")
   if not identical(ret, test.output) then
      local caller = assert(debug.getinfo(2))
      print("-- Fail at line " .. caller.currentline)
      print("-> Output:")
      print(json.stringify(ret))
      print("-> Expected:")
      print(json.stringify(test.output))
   end
end

-- Ensure that we detect abbreviations of the form <meaning> (<abbr>).
check{
   input = [[
   Die Versuche des dänischen Anti-Torture-Research-Teams (ATR) wurden in den
   darauffolgenden Jahren ohne die Beteiligung von Amnesty International
   weitergeführt.
   ]],
   output = {
      "ATR", "Anti-Torture-Research-Teams",
   }
}

-- Ensure that we detect abbreviations of the form <abbr> (<meaning>).
check{
   input = [[
   So ist LOL (Laughing Out Loud) die Bezeichnung, wenn ein Chatter lachen muss.
   ]],
   output = {
      "LOL", "Laughing Out Loud",
   },
}

-- Ensure that we can detect several abbreviations, when possible.
check{
   input = [[
   Zu den künstlichen Antioxidationsmitteln zählen die Gallate,
   Butylhydroxyanisol (BHA) und Butylhydroxytoluol (BHT).
   ]],
   output = {
      "BHA", "Butylhydroxyanisol",
      "BHT", "Butylhydroxytoluol",
   },
}

-- Ensure that matching is case-insensitive.
check{
   input = [[
   Ce dispositif soumet le versement de certaines aides communautaires au
   respect d'exigences en matière de bonnes conditions agricoles et
   environnementales (BCAE), de santé, et de protection animale.
   ]],
   output = {
      "BCAE", "bonnes conditions agricoles et environnementales",
   }
}

-- Ensure that matching is diacritics-insensitive.
check{
   input = [[
   Il existe un modèle macroéconomique, développé par l'OCDE, qui sert de
   référence en Europe, pour la mesure de ces « pressions environnementales » :
   le modèle Pression-État-Réponse (PER).
   ]],
   output = {
      "PER", "Pression-État-Réponse",
   }
}

-- Ensure that matching also works for scripts that cannot be converted to an
-- ASCII representation.
check{
   input = [[Автоматическая качающаяся аэрофотоустановка (АКАФУ)]],
   output = {
      "АКАФУ", "Автоматическая качающаяся аэрофотоустановка",
   }
}

-- Ensure that we match correctly any type of bracket.
check{
   input = [[
      Pression-État-Réponse (PER)
      Pression-État-Réponse [PER]
      Pression-État-Réponse {PER}
   ]],
   output = {
      "PER", "Pression-État-Réponse",
      "PER", "Pression-État-Réponse",
      "PER", "Pression-État-Réponse",
   }
}

-- Ensure that we handle nested brackets correctly when selecting the
-- abbreviation meaning.
check{
   input=  [[
   Die Europäische Gemeinschaft fokussierte ihre Aktivitäten stark auf den
   Agrarsektor (Gemeinsame Agrarpolitik (GAP)); in vielen Ländern begann eine
   Nettozahlerdebatte, die jahrzehntelang anhielt.
   ]],
   output = {
      "GAP", "Gemeinsame Agrarpolitik",
   },
}

-- Ensure that we filter out expansions containing unmatched brackets, for the
-- reasons given in mb_abbr.c.
check{
   input = [[
   In Bangladesh operano il Communist Party of Bangla Desh (Marxist-Leninist)
   abbreviato in BSD (ML) e il Proletarian Party of Purba Bangla abbreviato in
   BPSP.
   ]],
   output = {
      -- Would be "ML", "Marxist-Leninist) abbreviato in BSD" without filtering.
   },
}

-- Ensure that we don't filter out expansions containing matched brackets,
-- because some of them are legit.
check{
   input = [[
   Secondo il rapporto congiunto della Banca Mondiale e del Governo britannico,
   il solo settore forestale indonesiano sarebbe responsabile del rilascio in
   atmosfera di 2,563 MtCO2e ("Metric Tonne (ton) Carbon Dioxide Equivalent").
   ]],
   output = {
      "MtCO2e", "Metric Tonne (ton) Carbon Dioxide Equivalent"
   },
}

-- Ensure that we make the expansion start on a valid word boundary, where a
-- word is a sequence of alphanumeric characters or a hyphen.
check{
   input = [[
   Zu den synthetischen Aminosäuren gehört die 2-Amino-5-phosphonovaleriansäure
   (APV), ein Antagonist des NMDA-Rezeptors und das ökonomisch wichtige
   D-Phenylglycin ("R")-Phenylglycin, das in der Seitenkette vieler
   semisynthetischer β-Lactamantibiotica als Teilstruktur enthalten ist.
   ]],
   output = {
      "APV", "2-Amino-5-phosphonovaleriansäure",
   }
}
check{
   input = [[
   Esistono tre varianti principali attualmente in uso: lo Stinger base, lo
   "STINGER-Passive Optical Seeker Technique" (POST) e lo
   "STINGER-Reprogrammable Microprocessor" (RMP).
   ]],
   output = {
      "POST", "STINGER-Passive Optical Seeker Technique",
      "RMP", "STINGER-Reprogrammable Microprocessor",
   }
}

-- Ensure that we recognize the common pattern:
--    <elided_article>'<abbr> (<expansion>)
check{
   input = [[
   Dans seulement 9 % des cas, il s'agit d'EVASAN (Évacuations Sanitaires).
   ]],
   language = "fr",
   output = {
      "EVASAN", "Évacuations Sanitaires",
   },
}
check{
   input = [[
   La revue Communications de l'ACM (Association for Computing Machinery) a été
   consacrée principalement à la description et à l'analyse des algorithmes
   pendant plus de 20 ans.
   ]],
   language = "fr",
   output = {
      "ACM", "Association for Computing Machinery",
   },
}
check{
   input = [[
   Alle 21 il Commissario Straordinario Fantozzi insieme ai vertici dell'ENAC
   (Ente Nazionale Aviazione Civile) e Alitalia-Compagnia Aerea Italiana hanno
   iniziato a trasferire l'attività alla nuova Alitalia.
   ]],
   language = "it",
   output = {
      "ENAC", "Ente Nazionale Aviazione Civile",
   }
}
check{
   input = [[
   La città di Ascoli Piceno è stata ufficialmente proclamata "Città europea
   dello Sport" per l'anno 2014 dall'ACES ("Associazione delle Capitali
   Europee dello Sport").
   ]],
   language = "it",
   output = {
      "ACES", "Associazione delle Capitali Europee dello Sport"
   }
}
check{
   input = [[
   Per ovviare ai problemi che la mancanza di punti di riferimento fissi possono
   generare l'International Earth Rotation and Reference Systems Service (IERS)
   e l'Unione Astronomica Internazionale (UAI) hanno elaborato un modello
   matematico chiamato Sistema Internazionale di Riferimento Terrestre (ITRS).
   ]],
   language = "it",
   output = {
      "IERS", "International Earth Rotation and Reference Systems Service",
      "UAI", "Unione Astronomica Internazionale",
      "ITRS", "Internazionale di Riferimento Terrestre",
   }
}

-- Ensure that we truncate correctly an expansion when we have the pattern:
--    <abbreviation> (<expansion> <delimitor> <stuff>)
check{
   input = [[
      DELPHI (DEtector with Lepton, Photon and Hadron Identification : détecteur
      avec identification de leptons, de photons et de hadrons) est une
      expérience de physique des particules localisée au CERN, à Genève.
   ]],
   output = {
      "DELPHI", "DEtector with Lepton, Photon and Hadron Identification",
   }
}

-- If we encounter unmatched opening brackets, don't discard the whole sentence.
check{
   input = [[
      A B C D ( Institut national supérieur des arts du spectacle (INSAS)
   ]],
   output = {
      "INSAS", "Institut national supérieur des arts du spectacle",
   }
}

-- Ensure that we remove periods from the abbreviation.
check{
   input = [[
   International Rugby Board (I.R.B.)
   ]],
   output = {
      "IRB", "International Rugby Board",
   }
}

-- Ensure that we handle correctly the character X. 
check{
   input = [[
   In Eugene, Lane Transit District uses articulated buses on some high-traffic
   routes, as well as on their Emerald Express (EmX) Bus Rapid Transit Service.
   ]],
   output = {
      "EmX", "Emerald Express",
   }
}
check{
   input = [[
   The Inter-Asterisk eXchange (IAX2) protocol, RFC 5456, native to Asterisk,
   provides efficient trunking of calls among Asterisk PBXes, in addition to
   distributed configuration logic, and call completion to VoIP service
   providers who support it.
   ]],
   output = {
      "IAX2", "Inter-Asterisk eXchange",
   }
}
check{ 	 
   input = [[
   A VoIP-to-Telco gateway used to interface a VoIP PBX (private branch
   exchange) to analog lines would contain hybrids to perform the required
   conversion.
   ]],
   output = {
      "PBX", "private branch exchange",
   },
}

-- Ensure that we consider as word starters letters occuring after a -.
check{
   input = [[
   This pathway also generates dimethylallyl diphosphate, from pyruvate and
   D-glyercaldehyde 3-phosphate (GAP).
   ]],
   output = {
      "GAP", "D-glyercaldehyde 3-phosphate",
   },
}
check{
   input = [[
   In 1996, he signed the Comprehensive Nuclear-Test-Ban Treaty (CTBT) in New
   York, and in 2005 the Treaty on a Nuclear-Weapons-Free Zone in Central Asia
   (CANWFZ) in Semipalatinsk.
   ]],
   output = {
      "CTBT", "Comprehensive Nuclear-Test-Ban Treaty",
   },
}
