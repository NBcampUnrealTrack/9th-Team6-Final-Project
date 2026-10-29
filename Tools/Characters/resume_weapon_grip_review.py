source=(ROOT/'Tools/Characters/review_weapon_grips.py').read_text(encoding='utf-8')
exec(compile(source[source.index('def begin_case('):source.index('\ndef tick(')],'<grip-case>','exec'),globals())
STATE['case']=0;STATE['report']=[]
begin_case(0)
