-- converts binary files to quartus MIF files

import System.Environment
import System.IO
import Data.List
import Data.Word
import qualified Data.ByteString as B
import qualified Numeric as N

data MyWordType = MyByte | MyWord | MyLong deriving (Show)



main = do
       args <- getArgs
       let (wordtype, infile, outfile) = parseArgs args

       indata <- B.readFile infile
       let outdata = buildMif indata wordtype

       writeFile outfile outdata




-- parse arguments and return full info
parseArgs :: [String] -> (MyWordType, String, String)
parseArgs []        = error "empty args! must be bin2mif [-b|-w|-l|--byte|--word|--long] infile [outfile]"
parseArgs (x:[])    = (MyByte, x, x++".mif")
parseArgs (x:y:[])  = (parseHelp x, y, y++".mif")
parseArgs (x:y:z:_) = (parseHelp x, y, z)

-- parse helper
parseHelp :: String -> MyWordType
parseHelp "-b"      = MyByte
parseHelp "--byte"  = MyByte
parseHelp "-w"      = MyWord
parseHelp "--word"  = MyWord
parseHelp "-l"      = MyLong
parseHelp "--long"  = MyLong
parseHelp _         = error "only -b, -w, -l, --byte, --word, --long!"


-- MIF builder
buildMif :: B.ByteString -> MyWordType -> String

buildMif _ MyWord      =  error "words not supported yet!"
buildMif _ MyLong      =  error "longs not supported yet!"
buildMif bytes _       =  (bmifHeader bytes)++(bmifBody bytes)++(bmifEnd)

-- header for bytewise mifs
bmifHeader :: B.ByteString -> String
bmifHeader b  = "--built by bin2mif.hs\n\n"++
                "WIDTH=8;\n"++
		"DEPTH="++(show $ B.length b)++";\n\n"++
		"ADDRESS_RADIX=HEX;\nDATA_RADIX=HEX;\n\n"++
		"CONTENT BEGIN\n"

-- end for bytewise mifs
bmifEnd :: String
bmifEnd = "END;\n"

-- body for bytewise mifs
bmifBody :: B.ByteString -> String
bmifBody b  = bmifEmit.reverse $ foldl bmifMkAddrLen [] $ B.group b

bmifMkAddrLen :: [(Int,Int,Word8)] -> B.ByteString -> [(Int,Int,Word8)]
bmifMkAddrLen []                       b =  [(0,(B.length b),(B.head b))]
bmifMkAddrLen x@((oldaddr,oldlen,_):_) b =  let len = B.length b
                                            in [(oldaddr+oldlen,len,(B.head b))]++x

bmifEmit :: [(Int,Int,Word8)] -> String
bmifEmit []   = ""
bmifEmit ((addr,len,byte):xs) = (bmifMkLine addr len byte)++(bmifEmit xs)

bmifMkLine :: Int -> Int -> Word8 -> String
bmifMkLine addr len byte | len==1     = "\t"++(N.showHex addr "")++" : "++(N.showHex (fromIntegral byte) "")++";\n"
                         | otherwise  = "\t["++
                                        (N.showHex addr "")++".."++
					(N.showHex (addr+len-1) "")++"] : "++
					(N.showHex (fromIntegral byte) "")++";\n"


			 
